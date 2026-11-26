#include "survival.hpp"
#include <iostream>
#include <stdexcept>
using namespace genesis;
static void check(bool ok,const char* why) {
    if(!ok) throw std::runtime_error(why);
}
static std::uint64_t total(const World& w) {
    std::uint64_t s=0;
    for(const auto& t:w.tiles) s+=t.food+t.water;
    return s;
}
static bool equal(const World& a,const World& b) {
    if(a.tick!=b.tick || a.seed!=b.seed) return false;
    for(std::size_t i=0;i<stream_count;++i)
        if(a.streams[i].state!=b.streams[i].state) return false;
    for(std::size_t i=0;i<a.tiles.size();++i) {
        const auto& x=a.tiles[i]; const auto& y=b.tiles[i];
        if(x.food!=y.food || x.water!=y.water || x.wood!=y.wood || x.stone!=y.stone) return false;
    }
    for(std::size_t i=0;i<2;++i) {
        const auto& x=a.founders[i]; const auto& y=b.founders[i];
        if(x.id!=y.id || x.x!=y.x || x.y!=y.y || x.hunger!=y.hunger ||
           x.thirst!=y.thirst || x.action!=y.action || x.personality!=y.personality) return false;
    }
    return true;
}
int main() {
    try {
        World a{}, b{};
        initialize(a,42); initialize(b,42);
        auto previous=total(a);
        std::uint64_t consumed=0;
        for(int i=0;i<1000;++i) {
            check(step_survival(a)&&step_survival(b),"valid step");
            for(const auto& f:a.founders) {
                check(f.x<width && f.y<height && f.hunger<=100 && f.thirst<=100,"bounded state");
                if(f.action==Action::eat || f.action==Action::drink) ++consumed;
            }
        }
        check(consumed>0 && previous-total(a)==consumed,"resource conservation");
        check(equal(a,b),"same seed and tick replay");
        initialize(a,7); initialize(b,7);
        a.founders[0].personality[0]=0;
        b.founders[0].personality[0]=100;
        check(step_survival(a)&&step_survival(b),"controlled trait step");
        check(a.founders[0].action==Action::idle && b.founders[0].action==Action::move,"curiosity changes behavior");
        // Contention: one water unit, two thirsty founders, no neighboring stock.
        initialize(a,8);
        for(auto& t:a.tiles) t.water=0;
        for(auto& f:a.founders) { f.x=0; f.y=0; f.thirst=100; }
        a.tiles[0].water=1;
        check(step_survival(a),"contention");
        check(a.tiles[0].water==0 && a.founders[0].action==Action::drink &&
              a.founders[1].action==Action::idle && a.founders[1].thirst==100,"single consumption");
        // Corner movement toward water, no wrap.
        a.tiles[1].water=2;
        check(step_survival(a),"corner move");
        check(a.founders[1].x==1 && a.founders[1].y==0,"local water search");
        a.founders[1].x=width; b=a;
        check(!step_survival(a)&&equal(a,b),"invalid state atomic rejection");
        initialize(a,8);
        check(a.founders[0].hunger==0 && a.founders[0].action==Action::idle,"reset clears needs");
        a.tick=std::numeric_limits<std::uint64_t>::max(); b=a;
        check(!step_survival(a)&&equal(a,b),"overflow atomic rejection");
        std::cout<<"PASS: survival replay, conservation, trait behavior, contention, boundaries and rejection\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1;
    }
}
