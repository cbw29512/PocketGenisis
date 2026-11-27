#include "../storage/host_archive_checkpoint.hpp"
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
    std::string path="/tmp/genesis-handoff-XXXXXX";
    Directory(){if(!::mkdtemp(path.data())) throw io_error("create test directory");}
    ~Directory(){std::error_code ec;std::filesystem::remove_all(path,ec);}
};
int main() {
    try {
        Directory dir,legacy;
        auto original=std::make_unique<World>();initialize(*original,42);original->tick=100;
        original->archive_count=history_capacity;original->next_citizen_id=history_capacity+3;
        for(std::size_t i=0;i<original->archive_count;++i) {
            Citizen c{};c.id=i+3;c.parent1=1;c.parent2=2;c.birth_tick=1;
            c.health=0;c.thirst=100;original->archive[i]={c,2,DeathCause::dehydration};
        }
        check(save_host(legacy.path,*original,true).generation==1,"legacy saved");
        const auto initial=encode(*original,1);
        auto state=create_archive_host(dir.path,*original);
        check(state.generation==1&&state.history->count()==0&&state.world->archive_count==512,"initial archive checkpoint");
        rejects([&]{create_archive_host(dir.path,*original);},"no implicit replacement");
        // Simulate interruption after immutable segment publication but before checkpoint.
        publish_segment(dir.path,extract_segment(*original,0,64));
        auto interrupted=load_archive_host(dir.path);
        check(interrupted.history->count()==0&&interrupted.world->archive_count==512,"uncommitted batch does not retire RAM");
        auto next=retire_archive_host(dir.path,state,128);
        check(next.generation==2&&next.history->count()==128&&next.world->archive_count==384&&
              state.world->archive_count==512,"idempotent retry commits exact retirement");
        rejects([&]{retire_archive_host(dir.path,state,64);},"stale caller cannot overwrite");
        check(read_file(slot_path(dir.path,0)).bytes==state.committed,"previous checkpoint unchanged");
        auto final=retire_archive_host(dir.path,next,384);
        check(final.generation==3&&final.history->count()==512&&final.world->archive_count==0&&
              final.world->next_citizen_id==515,"all lifetime records retired with identity unchanged");
        auto recovered=load_archive_host(dir.path);
        for(std::size_t i=0;i<original->archive_count;++i) {
            const auto& d=original->archive[i];const auto* found=recovered.history->find(d.citizen.id);
            check(found&&death_payload(*found)==death_payload(d),"every historical field preserved");
        }
        check(read_file(slot_path(legacy.path,0)).bytes==initial&&encode(*original,1)==initial,"legacy and source untouched");
        check(!valid_population(*recovered.world),"legacy tick entry cannot bypass external validation");
        // A checksummed checkpoint referencing missing history is not silently rolled back.
        const auto missing=history_path(dir.path,384);
        std::filesystem::rename(missing,missing+".hold");
        rejects([&]{load_archive_host(dir.path);},"missing committed batch fails closed");
        std::filesystem::rename(missing+".hold",missing);
        // Torn latest checkpoint permits older read-only recovery, not automatic reuse.
        {std::ofstream out(slot_path(dir.path,final.slot),std::ios::trunc);out<<"torn";}
        auto degraded=load_archive_host(dir.path);
        check(degraded.degraded&&degraded.generation==2&&degraded.history->count()==128&&
              degraded.world->archive_count==384,"older checkpoint retains every identity");
        rejects([&]{retire_archive_host(dir.path,degraded,64);},"read-only fallback cannot reuse history");
        std::cout<<"PASS: archive watermark, interrupted handoff, exact retirement, recovery and fail-closed history\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
