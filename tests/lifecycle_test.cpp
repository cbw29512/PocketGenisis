#include "ecology.hpp"
#include <iostream>
#include <stdexcept>
using namespace genesis;
static void check(bool ok,const char* why) {
    if(!ok) throw std::runtime_error(why);
}
static void setup(World& w) {
    initialize(w,42);
    for(auto& f:w.founders) { f.personality.fill(50); f.personality[0]=0; }
}
int main() {
    try {
        World a{},b{};
        Production p{},q{};
        setup(a);setup(b);
        for(int i=0;i<100000 && !a.descendant_count;++i) {
            check(step_world(a,p)&&step_world(b,q),"autonomous world step");
            check(a.pregnancy.due_tick==b.pregnancy.due_tick && a.descendant_count==b.descendant_count,"gestation replay");
        }
        check(a.descendant_count==1 && b.descendant_count==1,"autonomous birth");
        auto& c=a.descendants[0]; const auto& d=b.descendants[0];
        check(c.id==3&&c.parent1==1&&c.parent2==2&&c.birth_tick==a.tick&&a.next_citizen_id==4,"stable genealogy");
        check(c.personality==d.personality&&c.birth_tick==d.birth_tick,"inheritance replay");
        check(c.personality[0]<=10,"mutation clamped");
        for(std::size_t i=1;i<c.personality.size();++i) check(c.personality[i]>=40&&c.personality[i]<=60,"mutation bounds");
        check(life_stage(c,a.tick)==LifeStage::infant&&c.purpose==Purpose::none,"child has no purpose");
        c.hunger=0;c.thirst=100;
        for(auto& f:a.founders) { f.x=c.x;f.y=c.y;f.hunger=0;f.thirst=0; }
        auto& tile=a.tiles[static_cast<std::size_t>(c.y)*width+c.x];tile.water=100;
        check(step_world(a,p)&&c.action==Action::drink,"parent provides dependent water");
        check(life_stage(c,c.birth_tick+2*ticks_per_year)==LifeStage::child,"child boundary");
        check(life_stage(c,c.birth_tick+12*ticks_per_year)==LifeStage::adolescent,"adolescent boundary");
        a.pregnancy={};
        a.tick=c.birth_tick+18*ticks_per_year-1;
        check(step_world(a,p)&&c.purpose!=Purpose::none,"adult purpose assigned");
        auto purpose=c.purpose;
        check(step_world(a,p)&&c.purpose==purpose,"purpose remains fixed");
        check(life_stage(a.founders[0],a.tick+100*ticks_per_year)==LifeStage::adult,"founder never ages physically");
        check(life_stage(c,c.birth_tick+55*ticks_per_year)==LifeStage::elder,"elder boundary");
        auto tick=a.tick;auto rng=a.streams[decisions].state;
        c.parent1=999;
        check(!step_world(a,p)&&a.tick==tick&&a.streams[decisions].state==rng,"invalid genealogy rejected atomically");
        setup(a);
        a.descendant_count=descendant_capacity;
        a.next_citizen_id=descendant_capacity+3;
        a.tick=599;
        for(std::size_t i=0;i<a.descendant_count;++i) {
            auto& child=a.descendants[i]; child.id=i+3;child.parent1=1;child.parent2=2;child.birth_tick=1;
        }
        check(step_world(a,p)&&a.descendant_count==descendant_capacity&&!a.pregnancy.due_tick,"capacity defers conception");
        setup(a);
        check(!a.descendant_count&&a.next_citizen_id==3&&!a.pregnancy.due_tick,"explicit reset clears population");
        std::cout<<"PASS: autonomous gestation, birth, genealogy, inheritance, maturation, shared survival, capacity and rejection\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;
    }
}
