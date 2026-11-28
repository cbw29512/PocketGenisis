#include "../core/walkable_admission.hpp"
#include <memory>
#include <iostream>
#include <stdexcept>
using namespace genesis;
namespace w=genesis::walkability;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto world=std::make_unique<World>();
        w::Map map{};w::Regions regions{};w::Scratch scratch{};
        map.fill(w::Surface::deep);
        map[0]=map[1]=map[3]=w::Surface::land;
        world->tiles[0]={20,20,20,20};world->tiles[1]={20,20,20,20};
        world->tiles[3]={100,100,100,100};
        world->tiles[2]={100,100,100,100};
        const w::Budget minimum{2,60,60,60,60};
        w::classify(map,regions,scratch);
        auto a=w::assess_start(*world,map,regions,0,1,minimum);
        check(a.same_region&&!a.accepted&&a.available.food==40,"remote resources excluded");
        check(!w::assess_start(*world,map,regions,1,3,minimum).same_region,"separated founders refused");
        map[2]=w::Surface::shallow;w::classify(map,regions,scratch);
        a=w::assess_start(*world,map,regions,0,1,minimum);
        check(a.accepted&&a.available.food==140&&a.available.land_tiles==3,"real crossing connects land; no shallow stock");
        check(!w::assess_start(*world,map,regions,1,2,minimum).accepted,"no founding on shallow water");
        check(!w::assess_start(*world,map,regions,0,3,minimum).accepted,"nonadjacent founders refused");
        check(!w::assess_start(*world,map,regions,0,0,minimum).accepted,"same tile refused");
        check(!w::assess_start(*world,map,regions,0,w::count,minimum).accepted,"invalid index refused");
        world->tick=1;
        check(!w::assess_start(*world,map,regions,0,1,minimum).accepted,"live world not readmitted");
        std::cout<<"PASS founding region resource isolation and crossing fixtures\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
