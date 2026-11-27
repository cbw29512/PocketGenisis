#include "ecology.hpp"
#include <iostream>
#include <stdexcept>
using namespace genesis;
static void check(bool ok,const char* why) {
    if(!ok) throw std::runtime_error(why);
}
// Choose a reproducible stream fixture, not a production override.
static std::uint64_t fixture(unsigned lo,unsigned hi) {
    for(std::uint64_t i=0;i<100000;++i) {
        Rng r{i};std::uint32_t n{};r.bounded(10000,n);
        if(n>=lo&&n<hi) return i;
    }
    throw std::runtime_error("could not find birth RNG fixture");
}
static void setup(World& w,std::uint64_t state) {
    initialize(w,42);w.tick=10;
    w.pregnancies[0]={1,2,11};w.pregnancy_count=1;
    for(auto& f:w.founders) f.personality[0]=0;
    w.streams[health_events].state=state;
}
int main() {
    try {
        World healthy{},weak{},fatal{};Production p{};
        setup(healthy,fixture(100,10000));setup(weak,fixture(20,100));setup(fatal,fixture(0,20));
        check(birth_risk(healthy.founders[0],healthy.founders[1])==100,"healthy baseline risk");
        auto stressed=healthy.founders[0];stressed.health=1;stressed.thirst=100;
        check(birth_risk(stressed,healthy.founders[1])>100&&birth_risk(stressed,healthy.founders[1])<=1500,"bounded stress risk");
        check(step_world(healthy,p)&&step_world(weak,p)&&step_world(fatal,p),"delivery ticks");
        check(healthy.descendant_count==1&&healthy.descendants[0].health==100&&healthy.descendants[0].birth_outcome==BirthOutcome::normal,"healthy birth");
        check(weak.descendant_count==1&&weak.descendants[0].health==35&&weak.descendants[0].birth_outcome==BirthOutcome::complicated,"complication");
        check(fatal.descendant_count==0&&fatal.archive_count==1&&fatal.next_citizen_id==4&&fatal.pregnancy_count==0,"fatal birth retains ID and releases active slot");
        const auto& d=fatal.archive[0];
        check(d.citizen.id==3&&d.citizen.parent1==1&&d.citizen.parent2==2&&d.death_tick==d.citizen.birth_tick&&d.cause==DeathCause::childbirth,"newborn history");
        check(healthy.descendants[0].personality==weak.descendants[0].personality&&healthy.descendants[0].personality==d.citizen.personality&&healthy.streams[reproduction].state==fatal.streams[reproduction].state,"health stream isolated from inheritance");
        check(valid_population(healthy)&&valid_population(weak)&&valid_population(fatal),"all outcomes valid");
        check(step_world(weak,p)&&weak.descendants[0].health==36&&weak.descendants[0].birth_outcome==BirthOutcome::complicated,"ordinary recovery preserves birth provenance");
        check(fatal.founders[0].health==100&&fatal.founders[1].health==100,"founders unaffected by newborn loss");
        fatal.archive[0].cause=DeathCause::dehydration;
        auto tick=fatal.tick;auto rng=fatal.streams[health_events].state;
        check(!step_world(fatal,p)&&fatal.tick==tick&&fatal.streams[health_events].state==rng,"inconsistent fatal outcome rejected atomically");
        setup(healthy,fixture(100,10000));healthy.descendants[0].birth_outcome=BirthOutcome::fatal;
        healthy.founders[0].birth_outcome=BirthOutcome::complicated;
        check(!step_world(healthy,p),"founder birth outcome rejected");
        std::cout<<"PASS: birth risk, newborn health and fatality, genealogy, isolated RNG, recovery and validation\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;
    }
}
