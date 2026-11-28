#include "../core/walkability.hpp"
#include <iostream>
#include <stdexcept>
using namespace genesis;
namespace w=genesis::walkability;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        w::Map map{};w::Regions regions{};w::Scratch scratch{};
        map.fill(w::Surface::land);
        w::classify(map,regions,scratch);
        check(regions.components==1&&w::connected(regions,0,w::count-1),"all land connected");
        for(unsigned y=0;y<height;++y) map[y*width+width/2]=w::Surface::deep;
        w::classify(map,regions,scratch);
        check(regions.components==2&&!w::connected(regions,0,width-1),"deep river splits land");
        check(!w::connected(regions,width/2,width/2),"deep water not walkable");
        check(!w::can_step(map,width/2-1,width/2),"walking into deep water forbidden");
        check(!w::can_step(map,width/2,width/2+1),"walking out of deep water is not a land move");
        check(w::can_step(map,0,1),"ordinary land step");
        check(!w::can_step(map,0,width+1),"diagonal jump forbidden");
        check(!w::can_step(map,width-1,width),"row wrap forbidden");
        check(!w::can_step(map,0,w::count),"invalid destination forbidden");
        map[width/2]=w::Surface::shallow;
        check(w::can_step(map,width/2-1,width/2)&&w::can_step(map,width/2,width/2+1),"shallow ford permits walking");
        w::classify(map,regions,scratch);
        check(regions.components==1&&w::connected(regions,0,width-1),"shallow crossing joins land");
        map.fill(w::Surface::deep);
        map[0]=map[width+1]=w::Surface::land;
        w::classify(map,regions,scratch);
        check(regions.components==2&&!w::connected(regions,0,width+1),"no diagonal crossing");
        map.fill(w::Surface::deep);
        map[width-1]=map[width]=w::Surface::land;
        w::classify(map,regions,scratch);
        check(regions.components==2&&!w::connected(regions,width-1,width),"no row wrap");
        map.fill(w::Surface::deep);
        w::classify(map,regions,scratch);
        check(regions.components==0&&!w::connected(regions,0,0),"all water");
        check(!w::connected(regions,w::count,0),"out of bounds");
        std::cout<<"PASS explicit walkability connectivity and barrier fixtures\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
