#include "ecology.hpp"
#include <iostream>
#include <stdexcept>
using namespace genesis;
static void check(bool ok,const char* why) {
    if(!ok) throw std::runtime_error(why);
}
static void setup(World& w) {
    initialize(w,42);w.tick=20*ticks_per_year;
    w.descendant_count=4;w.next_citizen_id=7;
    for(auto& f:w.founders) {f.personality[0]=0;f.health=40;}
    for(std::size_t i=0;i<4;++i) {
        auto& c=w.descendants[i];c.id=i+3;c.parent1=1;c.parent2=2;c.birth_tick=1;
        c.x=static_cast<std::uint16_t>(i<2?30:70);c.y=30;c.personality.fill(0);
        c.purpose=Purpose::gatherer;
    }
}
int main() {
    try {
        World w{};Production p{};setup(w);
        // Controlled opportunities, not a forced random outcome.
        for(int i=0;i<25&&w.pregnancy_count<2;++i) {
            w.tick+=600;attempt_conception(w);
        }
        check(w.pregnancy_count==2&&valid_population(w),"two families conceive independently");
        check(w.pregnancies[0].parent1!=w.pregnancies[1].parent1,"no duplicate participant");
        setup(w);w.pregnancies[0]={3,4,w.tick+1};w.pregnancies[1]={5,6,w.tick+1};w.pregnancy_count=2;
        check(step_world(w,p)&&w.descendant_count==6&&!w.pregnancy_count,"same-tick births");
        check(w.descendants[4].id==7&&w.descendants[4].parent1==3&&w.descendants[5].id==8&&
              w.descendants[5].parent1==5&&valid_population(w),"stable birth ordering and genealogy");
        setup(w);w.pregnancies[0]={3,4,w.tick+1};w.pregnancies[1]={5,6,w.tick+2};w.pregnancy_count=2;
        w.descendants[0].health=1;w.descendants[0].thirst=100;
        for(auto& t:w.tiles)t.water=0;
        check(step_world(w,p)&&w.pregnancy_count==1&&w.pregnancies[0].parent1==5&&
              w.lost_gestations==1&&w.archive_count==1,"loss compacts without cancelling unrelated pregnancy");
        check(step_world(w,p)&&!w.pregnancy_count&&w.descendant_count==4&&valid_population(w),"remaining birth succeeds");
        setup(w);w.pregnancies[0]={3,4,w.tick+1};w.pregnancies[1]={3,5,w.tick+1};w.pregnancy_count=2;
        auto tick=w.tick;auto rng=w.streams[decisions].state;
        check(!step_world(w,p)&&w.tick==tick&&w.streams[decisions].state==rng,"duplicate participant rejected atomically");
        setup(w);w.descendant_count=descendant_capacity-1;w.next_citizen_id=w.descendant_count+3;
        for(std::size_t i=4;i<w.descendant_count;++i) {
            auto& c=w.descendants[i];c.id=i+3;c.parent1=1;c.parent2=2;c.birth_tick=w.tick;
        }
        w.pregnancies[0]={3,4,w.tick+1};w.pregnancies[1]={5,6,w.tick+1};w.pregnancy_count=2;
        check(!step_world(w,p),"overbooked active capacity rejected");
        w.pregnancies[1]={};w.pregnancy_count=1;
        check(step_world(w,p)&&w.descendant_count==descendant_capacity&&valid_population(w),"reserved final slot delivered");
        std::cout<<"PASS: concurrent pregnancies, ordered delivery, isolated loss, duplicate and capacity rejection\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;
    }
}
