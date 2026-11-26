#include "ecology.hpp"
#include <iostream>
#include <stdexcept>
using namespace genesis;
static void check(bool ok,const char* why) {
    if(!ok) throw std::runtime_error(why);
}
static void setup(World& w) {
    initialize(w,42);w.tick=20*ticks_per_year;
    w.descendant_count=2;w.next_citizen_id=5;
    // Prevent founder conception in this controlled test by separating them.
    w.founders[0].x=0;w.founders[1].x=99;
    for(auto& f:w.founders) f.personality[0]=0;
    for(std::size_t i=0;i<2;++i) {
        auto& c=w.descendants[i];
        c.id=i+3;c.parent1=1;c.parent2=2;c.birth_tick=1;
        c.x=50;c.y=30;c.personality.fill(0);c.purpose=Purpose::gatherer;
    }
}
int main() {
    try {
        World a{},b{};Production p{},q{};
        setup(a);setup(b);
        // Hold local conditions constant for conception without forcing a draw.
        for(int i=0;i<100&&!a.pregnancy.due_tick;++i) {
            a.tick+=600;b.tick+=600;
            attempt_conception(a);attempt_conception(b);
        }
        check(a.pregnancy.parent1==3&&a.pregnancy.parent2==4,"descendants conceive");
        check(a.pregnancy.due_tick==b.pregnancy.due_tick,"seeded replay");
        a.tick=a.pregnancy.due_tick-1;b.tick=b.pregnancy.due_tick-1;
        check(step_world(a,p)&&step_world(b,q),"delivery tick");
        check(a.descendant_count==3&&a.descendants[2].id==5&&
              a.descendants[2].parent1==3&&a.descendants[2].parent2==4,"third generation genealogy");
        check(a.descendants[2].personality==b.descendants[2].personality,"inheritance replay");
        for(auto trait:a.descendants[2].personality) check(trait<=10,"actual parent traits");
        check(!a.pregnancy.due_tick&&valid_population(a),"valid multigeneration state");
        a.descendants[0].health=1;a.descendants[0].thirst=100;
        for(auto& t:a.tiles) t.water=0;
        check(step_world(a,p)&&a.archive_count==1&&valid_population(a),"parent death preserves child's genealogy");
        setup(a);a.pregnancy={3,4,a.tick+10};
        a.descendants[0].health=1;a.descendants[0].thirst=100;
        for(auto& t:a.tiles) t.water=0;
        check(step_world(a,p)&&!a.pregnancy.due_tick&&a.lost_gestations==1&&
              a.next_citizen_id==5&&a.archive_count==1,"parent death cancels gestation without consuming ID");
        setup(a);a.pregnancy={3,999,a.tick+1};
        auto tick=a.tick;
        check(!step_world(a,p)&&a.tick==tick,"missing gestation parent rejected");
        setup(a);a.descendants[0].parent1=3;
        check(!step_world(a,p),"invalid genealogy rejected");
        std::cout<<"PASS: multigeneration conception, inheritance, archived parents, pregnancy loss and validation\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;
    }
}
