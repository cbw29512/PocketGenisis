#include "ecology.hpp"
#include <iostream>
#include <stdexcept>
using namespace genesis;
static void check(bool ok,const char* why) {
    if(!ok) throw std::runtime_error(why);
}
static void setup(World& w) {
    initialize(w,44);w.tick=20*ticks_per_year;
    w.descendant_count=1;w.archive_count=2;w.next_citizen_id=6;
    for(auto& f:w.founders) {
        f.x=50;f.y=30;f.personality[0]=0;f.personality[2]=70;
    }
    for(std::size_t i=0;i<2;++i) {
        Citizen p{};p.id=i+3;p.parent1=1;p.parent2=2;p.birth_tick=1;
        p.health=0;p.thirst=100;p.purpose=Purpose::gatherer;
        w.archive[i]={p,w.tick-1,DeathCause::dehydration};
    }
    auto& c=w.descendants[0];c.id=5;c.parent1=3;c.parent2=4;
    c.birth_tick=w.tick-10;c.x=50;c.y=30;c.thirst=24;
    w.tiles[30*width+50].water=10;
}
int main() {
    try {
        World w{};Production p{};setup(w);
        auto& c=w.descendants[0];auto& tile=w.tiles[30*width+50];
        const auto assigned=w.tick;
        check(step_world(w,p)&&c.guardian_id==1&&c.guardian_since_tick==assigned,
              "successful care establishes guardian");
        check(valid_population(w)&&c.parent1==3&&c.parent2==4&&tile.water==9,"valid conserved adoption");
        w.founders[1].personality[2]=100;c.thirst=24;
        check(step_world(w,p)&&w.founders[0].action==Action::care&&c.guardian_id==1&&
              c.guardian_since_tick==assigned,"existing eligible guardian preferred");
        w.founders[0].thirst=24;c.thirst=24;
        check(step_world(w,p)&&w.founders[1].action==Action::care&&c.guardian_id==1,
              "temporary help does not replace living guardian");
        setup(w);tile.water=0;
        check(step_world(w,p)&&!c.guardian_id&&!c.guardian_since_tick,"no care no guardianship");
        setup(w);w.descendant_count=2;w.next_citizen_id=7;
        auto& g=w.descendants[1];g.id=6;g.parent1=1;g.parent2=2;g.birth_tick=1;
        g.x=50;g.y=30;g.purpose=Purpose::gatherer;g.personality[2]=90;
        for(auto& f:w.founders) f.personality[2]=0;
        check(step_world(w,p)&&c.guardian_id==6&&valid_population(w),"mortal adult guardian");
        g.health=1;g.thirst=100;
        for(auto& t:w.tiles)t.water=0;
        check(step_world(w,p)&&w.archive_count==3&&w.descendant_count==1&&
              c.guardian_id==6&&valid_population(w),"dead guardian retained as history");
        w.founders[0].personality[2]=80;tile.water=10;c.thirst=24;
        const auto replacement=w.tick;
        check(step_world(w,p)&&c.guardian_id==1&&c.guardian_since_tick==replacement&&
              valid_population(w),"successful replacement after death");
        w.tick=c.birth_tick+18*ticks_per_year;
        check(step_world(w,p)&&c.guardian_id==1&&c.purpose!=Purpose::none&&valid_population(w),
              "guardian identity persists after maturity");
        setup(w);c.guardian_id=999;c.guardian_since_tick=w.tick;
        const auto tick=w.tick;const auto rng=w.streams[decisions].state;
        check(!step_world(w,p)&&w.tick==tick&&w.streams[decisions].state==rng,"unknown guardian rejected atomically");
        setup(w);c.guardian_id=3;c.guardian_since_tick=w.tick;
        check(!step_world(w,p),"biological parent cannot be an adoptive guardian");
        setup(w);c.guardian_id=1;c.guardian_since_tick=c.birth_tick;
        check(!step_world(w,p),"guardianship before parental death rejected");
        std::cout<<"PASS: persistent guardianship, preference, replacement, historical identity and validation\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;
    }
}
