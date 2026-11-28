#include "ecology.hpp"
#include <iostream>
#include <stdexcept>
using namespace genesis;
static void check(bool ok,const char* why) { if(!ok) throw std::runtime_error(why); }
static std::uint64_t fixture(unsigned lo,unsigned hi) {
    for(std::uint64_t i=0;i<100000;++i) {
        Rng r{i};std::uint32_t roll{};r.bounded(10000,roll);
        if(roll>=lo&&roll<hi) return i;
    }
    throw std::runtime_error("missing disease RNG fixture");
}
static void setup(World& w) {
    initialize(w,42);w.tick=59;
    for(auto& f:w.founders) {f.x=50;f.y=30;f.personality[0]=0;f.diseases[0].resistance=0;}
    w.descendant_count=1;w.next_citizen_id=4;
    auto& c=w.descendants[0];c.id=3;c.parent1=1;c.parent2=2;c.birth_tick=1;
    c.x=52;c.y=30;c.diseases[0].resistance=0;
}
int main() {
    try {
        World w{};Production p{};setup(w);
        auto& a=w.founders[0];auto& b=w.founders[1];auto& c=w.descendants[0];
        a.diseases[0].illness=6;b.x=51;c.x=52;
        w.streams[disease_events].state=fixture(0,2000);
        // Isolated disease phase prevents infant movement from altering contacts.
        w.tick=60;advance_diseases(w);
        check(a.health==92&&a.diseases[0].illness==5,"existing illness damages health");
        check(b.diseases[0].illness==6&&!c.diseases[0].illness,"local transmission without same-pulse cascade");
        check(exposure_risk(1,100)<exposure_risk(1,0)&&exposure_risk(1,100)>0,"inherited resistance reduces risk without certainty");
        setup(w);a.diseases[0].illness=1;b.diseases[0].immunity=2;b.diseases[1].illness=3;
        w.streams[disease_events].state=fixture(0,7500);
        check(step_world(w,p)&&a.diseases[0].illness==0&&a.diseases[0].immunity==720,"recovery grants temporary immunity");
        check(b.diseases[0].immunity==1&&b.diseases[1].illness==2,"strain-specific protection");
        w.tick=119;check(step_world(w,p)&&!b.diseases[0].immunity,"immunity expires");
        setup(w);a.diseases[0].illness=1;
        w.streams[disease_events].state=fixture(7500,10000);
        check(step_world(w,p)&&!a.diseases[0].illness&&!a.diseases[0].immunity,"recovery does not guarantee immunity");
        setup(w);a.health=1;a.diseases[0].illness=4;c.health=1;c.diseases[1].illness=5;
        check(step_world(w,p)&&a.health==1&&w.descendant_count==0&&w.archive_count==1,"founder survives mortal disease death");
        check(w.archive[0].cause==DeathCause::disease&&w.archive[0].citizen.id==3&&valid_population(w),"disease death archives lineage");
        w.archive[0].citizen.diseases[1].illness=0;
        check(!valid_population(w),"disease death requires retained illness provenance");
        setup(w);a.diseases[0].illness=2;a.diseases[0].immunity=3;
        auto tick=w.tick;auto rng=w.streams[disease_events].state;
        check(!step_world(w,p)&&w.tick==tick&&w.streams[disease_events].state==rng,"invalid dual state rejected atomically");
        setup(w);a.diseases[0].resistance=20;b.diseases[0].resistance=40;
        a.diseases[0].immunity=50;Citizen newborn{};
        inherit_resistance(w,newborn,a,b);
        check(newborn.diseases[0].resistance>=20&&newborn.diseases[0].resistance<=40&&!newborn.diseases[0].immunity,"resistance inherited, acquired immunity not inherited");
        setup(w);w.tick=3599;
        w.streams[disease_events].state=fixture(5,10000);
        check(step_world(w,p)&&!is_ill(a)&&!is_ill(b)&&!is_ill(c),"environmental opportunity need not introduce disease");
        std::cout<<"PASS: transmission, resistance, recovery, strain-specific immunity, waning, mortality, inheritance and validation\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
