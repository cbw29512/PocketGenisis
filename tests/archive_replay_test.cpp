#include "../storage/host_archive_checkpoint.hpp"
#include <filesystem>
#include <iostream>
using namespace genesis;
using namespace genesis::storage;
static void check(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
template<class F> static void rejects(F f,const char* why) {
    try {f();} catch(const std::exception&) {return;}
    throw std::runtime_error(why);
}
struct Directory {
    std::string path="/tmp/genesis-replay-XXXXXX";
    Directory(){if(!::mkdtemp(path.data())) throw io_error("create test directory");}
    ~Directory(){std::error_code ec;std::filesystem::remove_all(path,ec);}
};
static std::uint64_t conception_seed() {
    for(std::uint64_t i=0;i<1000;++i) {
        Rng rng{i};std::uint32_t roll{};rng.bounded(4,roll);if(!roll) return i;
    }
    throw std::runtime_error("missing conception fixture");
}
int main() {
    try {
        Directory dir;
        auto original=std::make_unique<World>();initialize(*original,42);original->tick=599;
        for(auto& founder:original->founders) founder.personality[0]=0;
        original->streams[reproduction].state=conception_seed();
        original->archive_count=history_capacity;original->next_citizen_id=history_capacity+3;
        for(std::size_t i=0;i<original->archive_count;++i) {
            Citizen c{};c.id=i+3;c.parent1=1;c.parent2=2;c.birth_tick=1;
            c.health=0;c.thirst=100;original->archive[i]={c,2,DeathCause::dehydration};
        }
        check(valid_population(*original),"valid full archive fixture");
        auto start=create_archive_host(dir.path,*original);
        publish_segment(dir.path,extract_segment(*original,0,64));
        rejects([&]{advance_archive_host(dir.path,start,1);},"interrupted retirement blocks new ticks");
        auto retired=retire_archive_host(dir.path,start,512);
        check(retired.world->next_citizen_id==515&&retired.history->count()==512,"verified retirement");
        rejects([&]{advance_archive_host(dir.path,retired,0);},"zero interval refused");
        rejects([&]{advance_archive_host(dir.path,retired,archive_replay_limit+1);},"bounded replay interval");
        auto state=advance_archive_host(dir.path,retired,1);
        check(state.world->tick==600&&state.world->pregnancy_count==1,"actual conception after retirement");
        rejects([&]{advance_archive_host(dir.path,retired,1);},"stale checkpoint refused");
        const auto due=state.world->pregnancies[0].due_tick;
        while(state.world->tick<due) {
            const auto ticks=std::min(archive_replay_limit,due-state.world->tick);
            state=advance_archive_host(dir.path,state,ticks);
        }
        auto loaded=load_archive_host(dir.path);
        check(loaded.world->next_citizen_id==516&&loaded.history->count()==512,"actual 513th mortal birth persisted");
        check(loaded.world->descendant_count+loaded.world->archive_count==1,"new identity accounted for");
        const auto* newborn=live_citizen(*loaded.world,515);
        if(!newborn) {
            check(loaded.world->archive_count==1&&loaded.world->archive[0].citizen.id==515,"fatal birth retained");
            newborn=&loaded.world->archive[0].citizen;
        }
        check(newborn->parent1==1&&newborn->parent2==2&&newborn->birth_tick==due,"newborn lineage");
        for(std::size_t i=0;i<original->archive_count;++i) {
            const auto& d=original->archive[i];const auto* found=loaded.history->find(d.citizen.id);
            check(found&&death_payload(*found)==death_payload(d),"historic identity byte preservation");
        }
        check(valid_history_view(*loaded.world,*loaded.history)&&!valid_population(*loaded.world),"legacy validator stays closed");
        auto invented=std::make_unique<World>(*loaded.world);
        check(replay_archive(*invented,*loaded.history,1),"valid replay before mutation");
        invented->tiles[0].stone^=1;
        publish_file(dir.path,1-loaded.slot,encode_archive_checkpoint(*invented,*loaded.history,loaded.generation+1));
        rejects([&]{load_archive_host(dir.path);},"noncausal checkpoint rejected");
        std::cout<<"PASS: actual conception/delivery above lifetime ceiling, replay recovery, identity preservation and false advance rejection\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
