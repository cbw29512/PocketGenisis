#include "../storage/host_archive_cycle.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace genesis;
using namespace genesis::storage;
static void check(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
template<class F> static void rejects(F f,const char* why) {
    try {f();} catch(const std::exception&) {return;}
    throw std::runtime_error(why);
}
struct Directory {
    std::string path="/tmp/genesis-cycle-XXXXXX";
    Directory(){if(!::mkdtemp(path.data())) throw io_error("create test directory");}
    ~Directory(){std::error_code ec;std::filesystem::remove_all(path,ec);}
};
static std::uint64_t conception_seed() {
    for(std::uint64_t i=0;i<1000;++i) {
        Rng rng{i};std::uint32_t roll{};rng.bounded(4,roll);if(!roll) return i;
    }
    throw std::runtime_error("missing conception fixture");
}
static void setup(World& w) {
    initialize(w,42);w.tick=599;
    for(auto& founder:w.founders) founder.personality[0]=0;
    w.streams[reproduction].state=conception_seed();
    w.archive_count=history_capacity;w.next_citizen_id=history_capacity+3;
    for(std::size_t i=0;i<w.archive_count;++i) {
        Citizen c{};c.id=i+3;c.parent1=1;c.parent2=2;c.birth_tick=1;
        c.health=0;c.thirst=100;w.archive[i]={c,2,DeathCause::dehydration};
    }
}
int main() {
    try {
        Directory dir,conflict,corrupt,empty;
        auto original=std::make_unique<World>();setup(*original);
        auto initial=create_archive_host(dir.path,*original);
        rejects([&]{run_archive_cycle(dir.path,0);},"zero budget rejected");
        rejects([&]{run_archive_cycle(dir.path,cycle_tick_limit+1);},"excess budget rejected");
        rejects([&]{run_archive_cycle(empty.path);},"missing world never initialized");
        // Interruption after the first batch of a previous manual transaction.
        publish_segment(dir.path,extract_segment(*original,0,17));
        auto first=run_archive_cycle(dir.path,1);
        check(first.action==CycleAction::resumed_retirement&&first.retired==17&&
              first.state.history->count()==17&&first.state.world->tick==599,"resume exact partial batch");
        check(first.state.world->archive_count==495,"resident tail retained");
        for(unsigned i=0;i<7;++i) {
            auto cycle=run_archive_cycle(dir.path,1);
            check(cycle.action==CycleAction::retired&&cycle.retired==64&&
                  cycle.state.history->count()==17+64*(i+1)&&cycle.state.world->tick==599,"bounded retirement first");
        }
        auto advanced=run_archive_cycle(dir.path,1);
        check(advanced.action==CycleAction::advanced&&advanced.state.world->tick==600&&
              advanced.state.world->pregnancy_count==1&&advanced.state.history->count()==465&&
              advanced.state.world->archive_count==47,"automatic capacity recovery permits conception");
        auto loaded=load_archive_host(dir.path);
        for(std::size_t i=0;i<original->archive_count;++i) {
            const auto& d=original->archive[i];
            const auto* found=i<465?loaded.history->find(d.citizen.id):&loaded.world->archive[i-465];
            check(found&&death_payload(*found)==death_payload(d),"all identities preserved");
        }
        const auto due=loaded.world->pregnancies[0].due_tick;
        // The normal scheduler cadence continues until the existing gestation resolves.
        while(loaded.world->tick<due) {
            auto next=run_archive_cycle(dir.path,std::min(cycle_tick_limit,due-loaded.world->tick));
            check(next.action==CycleAction::advanced,"no unnecessary retirement below threshold");
            loaded=std::move(next.state);
        }
        check(loaded.world->next_citizen_id==516&&valid_history_view(*loaded.world,*loaded.history),"513th mortal identity through automatic cycle");
        // Conflicting pending bytes fail before changing the committed checkpoint.
        auto base=create_archive_host(conflict.path,*original);
        auto wrong=extract_segment(*original,0,64);wrong.records[0].citizen.thirst=99;
        publish_segment(conflict.path,wrong);
        rejects([&]{run_archive_cycle(conflict.path);},"conflicting segment rejected");
        check(load_archive_host(conflict.path).committed==base.committed,"conflict leaves prior state intact");
        auto bad=create_archive_host(corrupt.path,*original);
        {std::ofstream out(history_path(corrupt.path,0));out<<"torn";}
        rejects([&]{run_archive_cycle(corrupt.path);},"corrupt pending batch rejected");
        check(load_archive_host(corrupt.path).committed==bad.committed,"corruption leaves prior state intact");
        {std::ofstream out(slot_path(dir.path,loaded.slot),std::ios::trunc);out<<"torn";}
        check(load_archive_host(dir.path).degraded,"torn checkpoint recovers read-only");
        rejects([&]{run_archive_cycle(dir.path);},"degraded state never auto-advances");
        std::cout<<"PASS: bounded automatic archival, interrupted batch retry, actual birth and fail-closed recovery\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
