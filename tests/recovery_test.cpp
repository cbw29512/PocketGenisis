#include "ecology.hpp"
#include <iostream>
#include <stdexcept>
using namespace genesis;
static void check(bool ok,const char* why) {
    if(!ok) throw std::runtime_error(why);
}
int main() {
    try {
        World w{};initialize(w,42);
        auto& a=w.founders[0];auto& b=w.founders[1];
        a.x=10;a.y=10;b.x=20;b.y=10;
        a.personality[0]=100;b.personality[0]=100;
        auto rng=w.streams[decisions].state;
        for(int i=0;i<8;++i) check(step_survival(w),"reunion step");
        check(a.x==10&&b.x==12&&a.y==10&&b.y==10,"founders reunite deterministically");
        check(w.streams[decisions].state==rng&&!w.pregnancies[0].due_tick,"reunion is not a conception roll");
        initialize(w,42);
        for(auto& t:w.tiles) {t.water=0;t.food=0;}
        a.x=0;a.y=0;b.x=99;b.y=59;a.thirst=100;
        w.tiles[10].water=1;
        check(!find_resource(w,0,0,true).found,"local radius remains bounded");
        for(int i=0;i<10;++i) {
            act_survival(w,a);
            check(a.x==i+1&&a.y==0&&a.action==Action::move,"distant founder resource");
        }
        act_survival(w,a);
        check(a.action==Action::drink&&w.tiles[10].water==0&&a.thirst==88,"one distant unit consumed");
        initialize(w,42);
        for(auto& t:w.tiles) t.water=0;
        a.hunger=50;a.thirst=100;
        auto& tile=w.tiles[static_cast<std::size_t>(a.y)*width+a.x];tile.food=1;
        act_survival(w,a);
        check(a.action==Action::eat&&a.hunger==39&&a.thirst==100&&tile.food==0,"secondary urgent need served");
        for(auto& t:w.tiles) {t.food=0;t.water=0;}
        auto x=a.x,y=a.y;
        act_survival(w,a);
        check(a.action==Action::idle&&a.x==x&&a.y==y,"no invented resources");
        std::cout<<"PASS: founder reunion, distant resources, secondary needs, conservation\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;
    }
}
