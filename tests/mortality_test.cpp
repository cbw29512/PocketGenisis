#include "ecology.hpp"
#include <iostream>
#include <stdexcept>
using namespace genesis;
static void check(bool ok,const char* msg) {
    if(!ok) throw std::runtime_error(msg);
}
static Citizen person(std::uint64_t id,std::uint64_t birth) {
    Citizen c{};c.id=id;c.parent1=1;c.parent2=2;c.birth_tick=birth;
    c.x=0;c.y=0;c.personality.fill(0);return c;
}
int main() {
    try {
        World w{};Production p{};
        initialize(w,42);w.tick=70*ticks_per_year;
        w.descendant_count=3;w.next_citizen_id=6;
        w.descendants[0]=person(3,1);
        w.descendants[0].purpose=Purpose::gatherer;
        w.descendants[1]=person(4,w.tick);
        w.descendants[1].health=1;w.descendants[1].thirst=100;
        w.descendants[2]=person(5,w.tick);
        for(auto& t:w.tiles) t.water=0;
        for(auto& f:w.founders) {f.health=1;f.thirst=100;}
        check(step_world(w,p),"mortality tick");
        check(w.archive_count==2&&w.descendant_count==1&&w.descendants[0].id==5,"archive and compact");
        check(w.archive[0].citizen.id==3&&w.archive[0].cause==DeathCause::old_age,"old age");
        check(w.archive[1].citizen.id==4&&w.archive[1].cause==DeathCause::dehydration,"dehydration");
        check(w.archive[1].citizen.parent1==1&&w.archive[1].citizen.parent2==2,"genealogy retained");
        check(w.next_citizen_id==6&&w.founders[0].health==1&&w.founders[1].health==1,"founders immortal");
        for(auto& f:w.founders) {f.hunger=0;f.thirst=0;f.personality[0]=0;}
        check(step_world(w,p)&&w.founders[0].health==2,"founder health recovery");
        w.pregnancy={1,2,w.tick+1};
        check(step_world(w,p)&&w.descendant_count==2&&w.descendants[1].id==6&&w.next_citizen_id==7,"no ID reuse after death");
        check(w.descendants[1].lifespan_years>=60&&w.descendants[1].lifespan_years<=80,"seeded lifespan bounds");
        auto tick=w.tick;auto rng=w.streams[decisions].state;
        w.archive[1].citizen.id=3;
        check(!step_world(w,p)&&w.tick==tick&&w.streams[decisions].state==rng,"duplicate archive ID rejected");
        initialize(w,7);w.tick=599;
        w.archive_count=history_capacity;w.next_citizen_id=history_capacity+3;
        for(std::size_t i=0;i<history_capacity;++i) {
            auto c=person(i+3,1);c.health=0;c.thirst=100;
            w.archive[i]={c,2,DeathCause::dehydration};
        }
        check(step_world(w,p)&&w.archive_count==history_capacity&&!w.pregnancy.due_tick,"full history defers birth");
        initialize(w,7);check(w.archive_count==0&&w.next_citizen_id==3,"explicit reset");
        std::cout<<"PASS: death causes, founder recovery, immutable IDs, genealogy, capacity and invalid archive rejection\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;
    }
}
