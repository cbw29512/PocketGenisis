#include "../storage/host_history.hpp"
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
    std::string path="/tmp/genesis-history-XXXXXX";
    Directory(){if(!::mkdtemp(path.data())) throw io_error("create test directory");}
    ~Directory(){std::error_code ec;std::filesystem::remove_all(path,ec);}
};
int main() {
    try {
        Directory dir;World w{};initialize(w,42);w.tick=100;
        w.archive_count=history_capacity;w.next_citizen_id=history_capacity+3;
        for(std::size_t i=0;i<w.archive_count;++i) {
            Citizen c{};c.id=i+3;c.parent1=1;c.parent2=2;c.birth_tick=1;
            c.health=0;c.thirst=100;w.archive[i]={c,2,DeathCause::dehydration};
        }
        check(valid_population(w),"legal full archive");
        const auto original=encode(w,1);
        for(std::size_t i=0;i<w.archive_count;i+=segment_capacity) {
            const auto s=extract_segment(w,i,segment_capacity);
            publish_segment(dir.path,s);publish_segment(dir.path,s);
            check(encode_segment(load_segment(dir.path,w.seed,i))==encode_segment(s),"exact immutable segment roundtrip");
        }
        check(encode(w,1)==original,"export does not discard RAM history");
        auto s=extract_segment(w,0,segment_capacity);auto bytes=encode_segment(s);
        s.records[0].citizen.thirst=99;
        rejects([&]{publish_segment(dir.path,s);},"conflicting rewrite refused");
        check(read_file(history_path(dir.path,0)).bytes==bytes,"original bytes untouched");
        rejects([&]{load_segment(dir.path,43,0);},"foreign seed refused");
        rejects([&]{load_segment(dir.path,42,512);},"missing segment refused");
        rejects([&]{extract_segment(w,511,2);},"invalid range refused");
        auto damaged=bytes;damaged[30]^=1;
        rejects([&]{decode_segment(damaged);},"CRC mismatch refused");
        damaged=bytes;damaged.pop_back();
        rejects([&]{decode_segment(damaged);},"truncated segment refused");
        s.records[1]=s.records[0];
        rejects([&]{encode_segment(s);},"duplicate identity refused");
        // Independent synthetic records prove storage ordinals can exceed the RAM cap.
        s=extract_segment(w,0,segment_capacity);s.first=512;
        for(auto& d:s.records) {d.citizen.id+=512;d.death_tick=3;}
        publish_segment(dir.path,s);
        check(load_segment(dir.path,42,512).records.back().citizen.id==578,"external segment beyond 512 identities");
        {std::ofstream out(history_path(dir.path,512),std::ios::trunc);out<<"torn";}
        rejects([&]{load_segment(dir.path,42,512);},"corrupt file refused");
        rejects([&]{publish_segment(dir.path,s);},"corrupt existing history never overwritten");
        std::cout<<"PASS: immutable history batches, CRC, readback, conflict protection and ordinals beyond RAM cap\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
