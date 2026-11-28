#include "../storage/history_index.hpp"
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
    std::string path="/tmp/genesis-index-XXXXXX";
    Directory(){if(!::mkdtemp(path.data())) throw io_error("create test directory");}
    ~Directory(){std::error_code ec;std::filesystem::remove_all(path,ec);}
};
static DeathRecord person(std::uint64_t id,std::uint64_t birth,std::uint64_t death) {
    Citizen c{};c.id=id;c.parent1=1;c.parent2=2;c.birth_tick=birth;c.health=0;c.thirst=100;
    return {c,death,DeathCause::dehydration};
}
int main() {
    try {
        Directory dir;World w{};initialize(w,42);w.tick=25*ticks_per_year;
        // Synthetic valid chronology: parents 3/4 live until their child is born.
        const auto adult=20*ticks_per_year;
        for(std::uint64_t first=0;first<576;first+=64) {
            HistorySegment s{42,first,{}};
            for(std::uint64_t i=0;i<64;++i) {
                auto d=person(first+i+3,1,adult+100);
                if(d.citizen.id==578) {
                    d.citizen.birth_tick=adult;
                    d.citizen.parent1=3;d.citizen.parent2=4;
                }
                s.records.push_back(d);
            }
            publish_segment(dir.path,s);
        }
        auto index=HistoryIndex::load(dir.path,42,576);
        check(index.count()==576&&index.find(578)&&!index.find(579),"lookup above former cap");
        w.descendant_count=1;w.next_citizen_id=580;
        auto& c=w.descendants[0];c.id=579;c.parent1=3;c.parent2=4;c.birth_tick=adult;
        check(valid_history_view(w,index),"external parent chronology with 576 archived identities");
        c.birth_tick=adult+101;check(!valid_history_view(w,index),"parent died before birth");
        c.birth_tick=adult;c.guardian_id=1;c.guardian_since_tick=adult+101;
        check(valid_history_view(w,index),"archived parent and living founder guardian");
        c.guardian_since_tick=adult+99;check(!valid_history_view(w,index),"guardian assigned before parent death");
        c.guardian_id=0;c.guardian_since_tick=0;
        c.id=578;check(!valid_history_view(w,index),"live and archived identity collision");c.id=579;
        ++w.next_citizen_id;check(!valid_history_view(w,index),"missing identity rejected");--w.next_citizen_id;
        w.archive_count=1;w.next_citizen_id=581;
        w.archive[0]=person(580,1,adult+101);
        check(valid_history_view(w,index),"RAM tail joins external history");
        w.archive[0].citizen.id=578;check(!valid_history_view(w,index),"tail duplicate rejected");
        w.archive[0]=person(580,1,adult+99);check(!valid_history_view(w,index),"tail chronology rejected");
        rejects([&]{HistoryIndex::load(dir.path,42,575);},"partial segment boundary rejected");
        rejects([&]{HistoryIndex::load(dir.path,42,640);},"missing segment rejected");
        rejects([&]{HistoryIndex::load(dir.path,43,576);},"foreign seed rejected");
        Directory duplicate;publish_segment(duplicate.path,{42,0,{person(3,1,2)}});
        publish_segment(duplicate.path,{42,1,{person(3,1,3)}});
        rejects([&]{HistoryIndex::load(duplicate.path,42,2);},"cross segment duplicate rejected");
        Directory order;publish_segment(order.path,{42,0,{person(3,1,3)}});
        publish_segment(order.path,{42,1,{person(4,1,2)}});
        rejects([&]{HistoryIndex::load(order.path,42,2);},"cross segment chronology rejected");
        Directory invalid;
        publish_segment(invalid.path,{42,0,{person(3,1,2),person(4,1,2)}});
        auto descendant=person(5,adult,adult+1);descendant.citizen.parent1=3;descendant.citizen.parent2=4;
        publish_segment(invalid.path,{42,2,{descendant}});
        auto broken=HistoryIndex::load(invalid.path,42,3);
        World small{};initialize(small,42);small.tick=adult+2;small.next_citizen_id=6;
        check(!valid_history_view(small,broken),"cross segment invalid parent chronology rejected");
        check(!valid_population(w),"prototype does not bypass runnable world cap");
        std::cout<<"PASS: external identity index, combined genealogy, gap/duplicate/chronology rejection beyond 512\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
