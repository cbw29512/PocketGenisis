#include "ecology.hpp"
#include <iostream>
#include <stdexcept>
using namespace genesis;
static void check(bool ok,const char* why) {
    if(!ok) throw std::runtime_error(why);
}
static std::uint64_t stocks(const World& w) {
    std::uint64_t sum=0;
    for(const auto& t:w.tiles) sum+=t.food+t.water+t.wood;
    return sum;
}
int main() {
    try {
        World w{};
        initialize(w,42);
        Production p{};
        for(auto& t:w.tiles) t={0,0,0,20};
        for(auto& f:w.founders) f.personality[0]=0;
        check(step_world(w,p)&&p.food==0&&p.water==0&&p.wood==0,"not renewal boundary");
        w.tick=59;
        check(step_world(w,p)&&p.food==2*w.tiles.size()&&p.water==3*w.tiles.size()&&p.wood==w.tiles.size(),"spring production");
        for(const auto& t:w.tiles) check(t.food==2&&t.water==3&&t.wood==1&&t.stone==20,"renewal and finite stone");
        w.tick=3*ticks_per_year/4+59;
        check(step_world(w,p)&&p.food==0&&p.water==2*w.tiles.size()&&p.wood==0,"winter production");
        std::uint16_t s=99;
        check(renew_stock(s,3)==1&&s==100,"cap");
        s=105;
        check(renew_stock(s,3)==0&&s==105,"preserve above cap");
        initialize(w,123);
        auto initial=stocks(w);
        std::uint64_t made=0,used=0;
        for(int i=0;i<1000;++i) {
            check(step_world(w,p),"world step");
            made+=p.food+p.water+p.wood;
            for(const auto& f:w.founders)
                if(f.action==Action::eat||f.action==Action::drink) ++used;
        }
        check(stocks(w)+used==initial+made,"production-consumption accounting");
        w.tick=std::numeric_limits<std::uint64_t>::max();
        initial=stocks(w);
        check(!step_world(w,p)&&p.food==0&&p.water==0&&p.wood==0&&stocks(w)==initial,"invalid tick no production");
        std::cout<<"PASS: seasonal renewal, cap, finite stone and resource accounting\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1;
    }
}
