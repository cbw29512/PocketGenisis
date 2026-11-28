#include "survival.hpp"
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
    auto& child=w.descendants[0];
    child.id=5;child.parent1=3;child.parent2=4;child.birth_tick=w.tick-10;
    child.x=50;child.y=30;child.thirst=24;
    w.tiles[30*width+50].water=10;
}
int main() {
    try {
        World w{};setup(w);
        auto& child=w.descendants[0];auto& tile=w.tiles[30*width+50];
        check(step_survival(w),"validated orphan tick");
        check(child.action==Action::drink&&child.thirst==13&&tile.water==9,"single real resource consumed");
        check(w.founders[0].action==Action::care&&w.founders[1].action==Action::idle,"stable carer selection");
        check(child.parent1==3&&child.parent2==4&&w.archive_count==2&&valid_population(w),"genealogy unchanged");
        setup(w);w.founders[1].personality[2]=90;
        check(step_survival(w)&&w.founders[1].action==Action::care,"compassion tie break");
        setup(w);for(auto& f:w.founders) f.personality[2]=49;
        check(step_survival(w)&&child.thirst==25&&tile.water==10,"no guaranteed care");
        setup(w);for(auto& f:w.founders) f.thirst=24;
        check(step_survival(w)&&child.thirst==25&&tile.water==8,"carer survival takes priority");
        setup(w);child.x=47;
        check(step_survival(w)&&child.x==48&&child.action==Action::move&&tile.water==10,"nearby approach, no remote feeding");
        setup(w);child.x=40;
        check(step_survival(w)&&child.x==40&&child.action==Action::idle,"outside local awareness");
        setup(w);tile.water=0;
        check(step_survival(w)&&child.thirst==25&&tile.water==0,"no free resources");
        setup(w);w.descendant_count=2;w.next_citizen_id=7;
        auto& own=w.descendants[1];own.id=6;own.parent1=1;own.parent2=2;
        own.birth_tick=w.tick-10;own.x=50;own.y=30;own.thirst=24;
        check(step_survival(w)&&own.action==Action::drink&&child.thirst==25&&tile.water==9,"biological dependents reserve care first");
        std::cout<<"PASS: local orphan care, stable choice, resource conservation, priority, movement and genealogy\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;
    }
}
