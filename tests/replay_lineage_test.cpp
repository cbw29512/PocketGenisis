#include "../storage/host_archive_checkpoint.hpp"
#include <filesystem>
#include <iostream>
using namespace genesis;
using namespace genesis::storage;
static void check(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
struct Directory {
    std::string path="/tmp/genesis-lineage-XXXXXX";
    Directory(){if(!::mkdtemp(path.data())) throw io_error("create test directory");}
    ~Directory(){std::error_code ec;std::filesystem::remove_all(path,ec);}
};
struct CountingIndex {
    const DiskHistoryIndex& disk;
    mutable std::size_t visits{},lookups{};
    std::uint64_t seed() const {return disk.seed();}
    std::uint64_t count() const {return disk.count();}
    const DeathRecord* last() const {return disk.last();}
    const DeathRecord* find(std::uint64_t id) const {++lookups;return disk.find(id);}
    template<class Visit> bool visit(const Visit& v) const {++visits;return disk.visit(v);}
};
int main() {
    try {
        Directory dir;
        constexpr std::uint64_t count=1024,adult=20*ticks_per_year;
        for(std::uint64_t first=0;first<count;first+=segment_capacity) {
            HistorySegment batch{42,first,{}};
            for(std::uint64_t i=0;i<segment_capacity;++i) {
                Citizen c{};c.id=first+i+3;c.parent1=1;c.parent2=2;
                c.birth_tick=1;c.health=0;c.thirst=100;
                batch.records.push_back({c,adult+100,DeathCause::dehydration});
            }
            publish_segment(dir.path,batch);
        }
        auto original=std::make_unique<World>();initialize(*original,42);
        original->tick=adult+200;original->next_citizen_id=count+4;original->descendant_count=1;
        auto& child=original->descendants[0];child.id=count+3;child.parent1=3;child.parent2=4;
        child.birth_tick=adult;child.guardian_id=1;child.guardian_since_tick=adult+101;
        child.x=original->founders[0].x;child.y=original->founders[0].y;
        auto disk=DiskHistoryIndex::load(dir.path,42,count,original->next_citizen_id);
        auto map=HistoryIndex::load(dir.path,42,count);
        check(valid_history_view(*original,disk),"valid external parents");
        auto cache=ReplayLineage::build(*original,disk);
        check(cache.reference_count()==2&&cache.valid(*original),"only direct external parents cached");
        auto corrupt=std::make_unique<World>(*original);
        corrupt->descendants[0].id=10;
        check(!cache.valid(*corrupt),"uncached historical collision rejected");
        auto reference=std::make_unique<World>(*original);
        auto actual=std::make_unique<World>(*original);
        CountingIndex tracked{disk};
        check(replay_archive(*actual,tracked,120),"cached replay");
        check(tracked.visits==3&&tracked.lookups<40,"storage reads bounded to replay boundaries");
        Production produced{};
        for(unsigned i=0;i<120;++i) check(step_archive_world(*reference,produced,map),"reference step");
        check(world_payload(*actual)==world_payload(*reference),"canonical replay parity");
        auto invalid=std::make_unique<World>(*original);invalid->descendants[0].parent2=2000;
        const auto before=world_payload(*invalid);
        check(!replay_archive(*invalid,disk,1)&&world_payload(*invalid)==before,"invalid origin not mutated");
        // The real checkpoint loader now uses the disk index, not the RAM map.
        const auto encoded=encode_archive_checkpoint(*original,disk,1);
        publish_file(dir.path,0,encoded);
        auto loaded=load_archive_host(dir.path);
        check(loaded.history->count()==count&&loaded.committed==encoded,"disk-backed checkpoint load");
        auto advanced=advance_archive_host(dir.path,loaded,120);
        check(world_payload(*advanced.world)==world_payload(*reference),"disk-backed checkpoint advancement");
        std::cout<<"PASS: bounded lineage cache, disk-backed checkpoint replay, reference parity and identity guards\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
