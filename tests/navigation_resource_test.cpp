#include "../core/navigation_resource.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
namespace n=genesis::navigation;
namespace w=genesis::walkability;
using namespace genesis;
static void check(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
int main(){
    try{
        auto terrain=std::make_unique<n::Terrain>();
        auto water=std::make_unique<n::WaterMap>();
        auto world=std::make_unique<World>();
        auto scratch=std::make_unique<n::Scratch>();
        for(unsigned y=0;y<height;++y){
            const auto i=y*width+1;
            terrain->surface[i]=w::Surface::deep;
            water->kind[i]=n::WaterKind::salt;water->depth[i]=10;
        }
        world->tiles[2].food=100;world->tiles[2*width].food=1;
        auto r=n::seek(*world,*terrain,*water,0,n::Need::food,*scratch,8);
        check(r.path.found&&r.destination==2*width&&r.path.next==width&&r.path.total==2,"unreachable food excluded");
        world->tiles[2*width].food=0;
        check(!n::seek(*world,*terrain,*water,0,n::Need::food,*scratch,8).path.found,"no false route");
        terrain->surface[width+1]=w::Surface::shallow;water->depth[width+1]=1;
        world->tiles[width+1].water=100;world->tiles[width+1].food=100;
        check(!n::seek(*world,*terrain,*water,0,n::Need::water,*scratch,8).path.found,"saltwater rejected");
        check(!n::seek(*world,*terrain,*water,0,n::Need::food,*scratch,7).path.found,"turn budget includes wading");
        r=n::seek(*world,*terrain,*water,0,n::Need::food,*scratch,8);
        check(r.path.found&&r.destination==2&&r.path.total==8,"ford route reaches dry food");
        water->kind[width+1]=n::WaterKind::fresh;
        r=n::seek(*world,*terrain,*water,0,n::Need::water,*scratch,4);
        check(r.path.found&&r.destination==width+1&&r.path.total==4,"freshwater reachable");
        world->tiles[0].water=1;
        r=n::seek(*world,*terrain,*water,0,n::Need::water,*scratch,0);
        check(r.path.found&&r.destination==0&&r.path.total==0&&r.path.next==0,"local stock no movement");
        check(world->tiles[0].water==1,"search never consumes stock");
        // A nearby mountain is more expensive than a farther flat resource.
        terrain->surface.fill(w::Surface::land);water->depth.fill(0);water->kind.fill(n::WaterKind::dry);
        world->tiles[width+1].food=0;world->tiles[1].food=1;world->tiles[2].food=0;world->tiles[2*width].food=1;
        terrain->cover[1]=n::Cover::highland;terrain->elevation[1]=20;
        r=n::seek(*world,*terrain,*water,0,n::Need::food,*scratch,8);
        check(r.path.found&&r.destination==2*width&&r.path.total==2,"cheaper flat route chosen");
        std::cout<<"PASS reachable resource targeting, water safety and weighted turn budgets\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
