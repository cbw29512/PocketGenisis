#include "../core/navigation_admission.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
namespace n=genesis::navigation;
namespace w=genesis::walkability;
using namespace genesis;
static void check(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
int main(){
    try{
        auto t=std::make_unique<n::Terrain>();
        auto water=std::make_unique<n::WaterMap>();
        auto scratch=std::make_unique<n::Scratch>();
        auto labels=std::make_unique<w::Regions>();
        auto flood=std::make_unique<w::Scratch>();
        auto world=std::make_unique<World>();
        t->elevation.fill(150);
        const auto basin=20*width+20;
        // Inland depression: lowest escape saddle at 120.
        t->elevation[basin]=100;
        for(unsigned x=0;x<20;++x)t->elevation[20*width+x]=120;
        t->elevation[0]=90;t->elevation[1]=103;
        check(n::generate_water(*t,*water,*scratch),"generate depths");
        check(water->kind[0]==n::WaterKind::salt&&water->depth[0]==15,"boundary ocean");
        check(water->kind[1]==n::WaterKind::salt&&water->depth[1]==2&&
              t->surface[1]==w::Surface::shallow,"coastal shallows");
        check(water->kind[basin]==n::WaterKind::fresh&&water->depth[basin]==20,"inland basin fills to saddle");
        check(water->kind[2]==n::WaterKind::dry&&!water->depth[2],"upland stays dry");
        const auto before=water->depth;
        check(!n::generate_water(*t,*water,*scratch,{130,100,2})&&water->depth==before,"invalid policy leaves map intact");
        // Cutting a below-sea-level channel converts basin to saltwater.
        for(unsigned x=0;x<20;++x)t->elevation[20*width+x]=100;
        check(n::generate_water(*t,*water,*scratch),"regenerate scratch candidate");
        check(water->kind[basin]==n::WaterKind::salt&&water->depth[basin]==5,"ocean-connected basin");
        // All-dry fixture with a cliff wall: remote food cannot qualify spawn.
        t->elevation.fill(110);t->surface.fill(w::Surface::land);
        water->depth.fill(0);water->kind.fill(n::WaterKind::dry);
        for(unsigned y=0;y<height;++y)for(unsigned x=3;x<width;++x)t->elevation[y*width+x]=150;
        world->tiles[0].food=4;world->tiles[1].food=5;world->tiles[3].food=100;
        const w::Budget minimum{1,10,0,0,0};
        auto a=n::assess(*world,*t,*water,*labels,*flood,0,1,minimum);
        check(a.same_region&&!a.accepted&&a.available.food==9,"cliff-separated food excluded");
        check(!n::assess(*world,*t,*water,*labels,*flood,2,3,minimum).same_region,"illegal founder edge rejected");
        world->tiles[1].food=6;
        check(n::assess(*world,*t,*water,*labels,*flood,0,1,minimum).accepted,"reachable stocks qualify");
        // Even walkable seawater is never counted as dry settlement stock.
        t->surface[1]=w::Surface::shallow;water->kind[1]=n::WaterKind::salt;water->depth[1]=1;
        world->tiles[1].water=100;
        check(!n::assess(*world,*t,*water,*labels,*flood,0,1,minimum).same_region,"wet spawn refused");
        auto b=n::assess(*world,*t,*water,*labels,*flood,0,width,{1,0,1,0,0});
        check(b.same_region&&!b.accepted&&b.available.water==0,"salt water not counted as drinking supply");
        std::cout<<"PASS ocean connectivity, freshwater depth and terrain-aware founder admission\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
