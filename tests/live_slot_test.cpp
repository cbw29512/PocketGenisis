#include "ecology.hpp"
#include <iostream>
#include <stdexcept>
using namespace genesis;
static void check(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
int main() {
    try {
        World w{};initialize(w,42);w.tick=20*ticks_per_year;
        // Legal world with a long death history and sparse active identities.
        w.archive_count=497;w.descendant_count=3;w.next_citizen_id=503;
        for(std::size_t i=0;i<w.archive_count;++i) {
            Citizen c{};c.id=i+3;c.parent1=1;c.parent2=2;c.birth_tick=1;
            c.health=0;c.thirst=100;w.archive[i]={c,2,DeathCause::dehydration};
        }
        for(std::size_t i=0;i<3;++i) {
            auto& c=w.descendants[i];c.id=500+i;c.parent1=1;c.parent2=2;
            c.birth_tick=1;c.x=50;c.y=30;c.purpose=Purpose::gatherer;
        }
        auto& child=w.descendants[2];child.parent1=500;child.parent2=501;
        child.birth_tick=w.tick-10;child.purpose=Purpose::none;child.thirst=24;
        for(auto& f:w.founders) {f.personality[0]=0;f.x=0;f.y=0;}
        w.pregnancies[0]={500,501,w.tick+10};w.pregnancy_count=1;
        check(valid_population(w),"sparse identities valid");
        check(live_slot(w,500)==2&&live_slot(w,501)==3&&live_slot(w,502)==4&&
              live_slot(w,499)==live_capacity&&live_slot(w,0)==live_capacity,"sparse slot lookup");
        auto& tile=w.tiles[30*width+50];tile.water=10;
        Production p{};check(step_world(w,p)&&child.thirst==13&&tile.water==9,"high-ID parent care");
        check(valid_population(w),"sparse state after tick");
        w.pregnancies[1]=w.pregnancies[0];w.pregnancy_count=2;
        check(!valid_population(w),"duplicate high-ID participants rejected");
        // Isolated lookup fixture beyond old cap; not a legal full world yet.
        w.descendants[0].id=1000000;w.descendants[1].id=1000001;w.descendants[2].id=1000002;
        check(live_slot(w,1000001)==3&&live_citizen(w,1000002)==&w.descendants[2]&&
              live_slot(w,500)==live_capacity,"lookup independent of lifetime ID magnitude");
        static_assert(CareRequests{}.size()==descendant_capacity+2);
        std::cout<<"PASS: bounded live slots, sparse identity care and gestation validation\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
