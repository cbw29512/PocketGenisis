#include "ecology.hpp"
#include <iostream>
#include <stdexcept>
using namespace genesis;
static void check(bool ok,const char* why) {
    if(!ok) throw std::runtime_error(why);
}
static void setup(World& w) {
    initialize(w,44);w.tick=10;w.descendant_count=1;w.next_citizen_id=4;
    for(auto& f:w.founders) { f.x=50;f.y=30;f.personality[0]=0; }
    auto& c=w.descendants[0];
    c.id=3;c.parent1=1;c.parent2=2;c.birth_tick=1;c.x=50;c.y=30;c.thirst=24;
}
int main() {
    try {
        World w{};Production p{};
        setup(w);
        auto& c=w.descendants[0];
        auto& tile=w.tiles[30*width+50];tile.water=10;
        check(step_world(w,p),"care tick");
        check(tile.water==9&&c.thirst==13&&c.action==Action::drink,"one unit consumed; no double feeding");
        check(w.founders[0].action==Action::care&&w.founders[1].action==Action::idle,"stable parent order");
        setup(w);
        w.founders[0].thirst=24;w.founders[1].thirst=24;tile.water=10;
        check(step_world(w,p)&&tile.water==8&&c.thirst==25&&c.action==Action::idle,"urgent parents meet own needs; infant cannot forage");
        setup(w);
        c.x=45;
        check(step_world(w,p)&&c.x==46&&c.action==Action::move&&c.thirst==25,"dependent transport, not remote feeding");
        setup(w);
        tile.water=0;
        check(step_world(w,p)&&c.thirst==25&&c.action==Action::idle,"no free water");
        setup(w);
        c.health=1;c.thirst=100;c.x=0;c.y=0;
        check(step_world(w,p)&&w.descendant_count==0&&w.archive_count==1&&
              w.archive[0].cause==DeathCause::dehydration,"unserved infant can die");
        setup(w);
        w.tick=c.birth_tick+2*ticks_per_year;
        tile.water=10;
        check(step_world(w,p)&&c.action==Action::drink&&tile.water==9,"older child can consume independently");
        std::cout<<"PASS: care priority, conserved stock, dependent movement, mortality and age transition\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;
    }
}
