#include "../storage/disk_history_index.hpp"
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
    std::string path="/tmp/genesis-disk-index-XXXXXX";
    Directory(){if(!::mkdtemp(path.data())) throw io_error("create test directory");}
    ~Directory(){std::error_code ec;std::filesystem::remove_all(path,ec);}
};
static DeathRecord person(std::uint64_t id,std::uint64_t birth,std::uint64_t death) {
    Citizen c{};c.id=id;c.parent1=1;c.parent2=2;c.birth_tick=birth;c.health=0;c.thirst=100;
    return {c,death,DeathCause::dehydration};
}
int main() {
    try {
        Directory dir,duplicate,wrong;
        constexpr std::uint64_t count=4096;
        constexpr auto adult=20*ticks_per_year;
        auto w=std::make_unique<World>();initialize(*w,42);w->tick=adult+200;
        // Deliberately leave ID 5 alive; historical identity != death ordinal.
        for(std::uint64_t first=0;first<count;first+=segment_capacity) {
            HistorySegment batch{42,first,{}};
            for(std::uint64_t i=0;i<segment_capacity;++i) {
                auto id=first+i+3;if(id>=5) ++id;
                auto d=person(id,1,adult+100);
                if(id==count+3) {d.citizen.parent1=3;d.citizen.parent2=4;d.citizen.birth_tick=adult;}
                batch.records.push_back(d);
            }
            publish_segment(dir.path,batch);
        }
        w->descendant_count=1;w->next_citizen_id=count+4;
        auto& live=w->descendants[0];live.id=5;live.parent1=1;live.parent2=2;
        live.birth_tick=adult;
        auto disk=DiskHistoryIndex::load(dir.path,42,count,w->next_citizen_id);
        auto memory=HistoryIndex::load(dir.path,42,count);
        check(disk.count()==count&&!disk.find(5)&&!disk.find(w->next_citizen_id),"sparse identity holes");
        check(valid_history_view(*w,disk)&&valid_history_view(*w,memory),"both validators agree");
        std::uint64_t seen=0;
        check(disk.visit([&](const DeathRecord& d){
            ++seen;const auto* expected=memory.find(d.citizen.id);
            return expected&&disk_row(d)==disk_row(*expected);
        })&&seen==count,"every row byte-identical to reference");
        live.parent1=3;live.parent2=4;live.birth_tick=adult+101;
        check(!valid_history_view(*w,disk)&&!valid_history_view(*w,memory),"dead parents rejected");
        live.parent1=1;live.parent2=2;live.birth_tick=adult;
        rejects([&]{DiskHistoryIndex::load(dir.path,42,count-1,w->next_citizen_id);},"partial prefix rejected");
        rejects([&]{DiskHistoryIndex::load(dir.path,43,count,w->next_citizen_id);},"foreign seed rejected");
        rejects([&]{DiskHistoryIndex::load(dir.path,42,count,UINT64_MAX);},"unbounded assigned range rejected");
        rejects([&]{DiskHistoryIndex::load(dir.path,42,count,count+3);},"out of range record rejected");
        publish_segment(duplicate.path,{42,0,{person(3,1,2)}});
        publish_segment(duplicate.path,{42,1,{person(3,1,3)}});
        rejects([&]{DiskHistoryIndex::load(duplicate.path,42,2,5);},"duplicate across batches");
        publish_segment(wrong.path,{42,0,{person(3,1,3)}});
        publish_segment(wrong.path,{42,1,{person(4,1,2)}});
        rejects([&]{DiskHistoryIndex::load(wrong.path,42,2,5);},"chronology conflict");
        std::filesystem::remove(history_path(dir.path,segment_capacity));
        rejects([&]{DiskHistoryIndex::load(dir.path,42,count,w->next_citizen_id);},"missing immutable segment");
        // Scratch rows detect damaged data and truncated reads, not missing IDs.
        std::string temp=dir.path+"/.row-test-XXXXXX";Fd fd(::mkstemp(temp.data()));
        check(fd.value>=0,"scratch test open");::unlink(temp.c_str());
        const auto row=person(3,1,2);
        check(::ftruncate(fd.value,static_cast<off_t>(disk_row_size()))==0,"scratch test size");
        write_disk_row(fd.value,row);DeathRecord out{};
        check(read_disk_row(fd.value,3,out)&&disk_row(out)==disk_row(row),"row roundtrip");
        const std::uint8_t bad=0xff;check(::pwrite(fd.value,&bad,1,0)==1,"damage scratch");
        rejects([&]{read_disk_row(fd.value,3,out);},"CRC damage rejected");
        check(::ftruncate(fd.value,1)==0,"truncate scratch");
        rejects([&]{read_disk_row(fd.value,3,out);},"truncated row rejected");
        std::cout<<"PASS: 4096 archived identities, disk/map parity, bounded scratch rows, corruption and genealogy\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
