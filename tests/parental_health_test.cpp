#include "ecology.hpp"
#include "../storage/history_segment.hpp"
#include <iostream>
using namespace genesis;
using namespace genesis::storage;
static void check(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
static std::uint64_t fixture(unsigned lo,unsigned hi,unsigned choice=2) {
    for(std::uint64_t i=0;i<100000;++i) {
        Rng r{i};std::uint32_t roll{},selected{};
        r.bounded(10000,roll);r.bounded(2,selected);
        if(roll>=lo&&roll<hi&&(choice==2||choice==selected)) return i;
    }
    throw std::runtime_error("missing event fixture");
}
static void setup(World& w) {
    initialize(w,42);w.tick=20*ticks_per_year+1;
    w.descendant_count=4;w.next_citizen_id=7;
    for(auto& f:w.founders) f.personality[0]=0;
    for(std::size_t i=0;i<4;++i) {
        auto& c=w.descendants[i];c.id=i+3;c.parent1=1;c.parent2=2;
        c.birth_tick=1;c.x=50;c.y=30;c.purpose=Purpose::gatherer;
    }
    w.pregnancies[0]={3,4,w.tick+1};w.pregnancy_count=1;
    w.streams[health_events].state=fixture(1500,10000);
}
int main() {
    try {
        auto w=std::make_unique<World>();setup(*w);Production p{};
        const auto risk=parental_risk(w->descendants[0],w->descendants[1]);
        check(risk==50,"provisional baseline");
        w->descendants[0].health=40;
        check(parental_risk(w->descendants[0],w->descendants[1])>risk,"risk responds to stress");
        w->descendants[0].health=100;
        auto normal=std::make_unique<World>(*w);
        normal->streams[parental_health_events].state=fixture(750,10000);
        w->streams[parental_health_events].state=fixture(5,50,1);
        check(step_world(*w,p)&&step_world(*normal,p),"delivery");
        check(w->descendant_count==5&&w->descendants[1].health==70&&
              w->descendants[0].health==100&&w->descendants[4].health==100,"one injured parent");
        check(w->descendants[4].personality==normal->descendants[4].personality&&
              w->streams[reproduction].state==normal->streams[reproduction].state&&
              w->streams[health_events].state==normal->streams[health_events].state,"RNG isolation");
        check(step_world(*w,p)&&w->descendants[1].health==71,"ordinary recovery");
        setup(*w);
        // A previously born citizen dies in mortality; the newborn and
        // another parent die at delivery. Archive must sort the tick suffix.
        w->descendants[3].health=1;w->descendants[3].thirst=100;
        for(auto& t:w->tiles)t.water=0;
        w->streams[health_events].state=fixture(0,20);
        w->streams[parental_health_events].state=fixture(0,5,0);
        check(step_world(*w,p)&&valid_population(*w),"fatal complication tick");
        check(w->archive_count==3&&w->archive[0].citizen.id==3&&
              w->archive[0].cause==DeathCause::parental_childbirth&&
              w->archive[1].citizen.id==6&&w->archive[2].citizen.id==7&&
              w->archive[2].cause==DeathCause::childbirth,"ordered three distinct deaths");
        check(w->next_citizen_id==8&&w->descendant_count==2,"no identities lost");
        const auto segment=extract_segment(*w,0,3);
        check(decode_segment(encode_segment(segment)).records[0].cause==DeathCause::parental_childbirth,
              "archive codec preserves cause");
        auto invalid=segment;invalid.records[2].cause=DeathCause::parental_childbirth;
        check(!valid_segment(invalid),"newborn and parent causes distinct");
        auto saved=encode(*w,1);
        check(decode(saved).world!=nullptr,"checkpoint accepts new death cause");
        setup(*w);w->pregnancies[0]={1,2,w->tick+1};
        w->streams[parental_health_events].state=fixture(0,5,0);
        check(step_world(*w,p)&&w->founders[0].health==1&&w->founders[1].health==100&&
              !w->archive_count&&valid_population(*w),"founder immortality preserved");
        setup(*w);w->pregnancies[1]={5,6,w->tick+1};w->pregnancy_count=2;
        w->streams[parental_health_events].state=fixture(0,5,0);
        check(step_world(*w,p)&&w->next_citizen_id==9&&!w->pregnancy_count&&valid_population(*w),
              "simultaneous disjoint pregnancies survive compaction");
        std::cout<<"PASS: parental injury, recovery, independent RNG, fatality, ordered history and immortality\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
