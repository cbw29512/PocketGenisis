#include "../core/terrain_travel.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
namespace n=genesis::navigation;
namespace w=genesis::walkability;
using namespace genesis;
static void check(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
int main(){
    try{
        // Large workspaces are caller-owned, not on an embedded task stack.
        auto t=std::make_unique<n::Terrain>();
        auto scratch=std::make_unique<n::Scratch>();
        check(n::surface(0,2)==w::Surface::land&&n::surface(2,2)==w::Surface::shallow&&
              n::surface(3,2)==w::Surface::deep,"depth boundaries");
        check(n::cost(*t,0,1)==1,"flat land");
        t->cover[1]=n::Cover::forest;
        check(n::cost(*t,0,1)==2,"forest penalty");
        t->cover[1]=n::Cover::highland;t->elevation[1]=20;
        check(n::cost(*t,0,1)==6&&n::cost(*t,1,0)==3,"directional mountain penalty");
        auto r=n::route(*t,0,2,*scratch);
        check(r.found&&r.total==4&&r.next==width,"prefer cheaper mountain detour");
        t->elevation[1]=33;
        check(!n::cost(*t,0,1)&&!n::cost(*t,1,0),"cliff blocks both ways");
        t->elevation.fill(0);t->cover.fill(n::Cover::open);
        for(unsigned y=0;y<height;++y)t->surface[y*width+1]=w::Surface::deep;
        check(!n::route(*t,0,2,*scratch).found,"deep river cannot be walked across");
        t->surface[width+1]=w::Surface::shallow;
        r=n::route(*t,0,2,*scratch);
        check(r.found&&r.next==width&&r.total==8,"route via shallow ford");
        check(!n::cost(*t,width-1,width),"no row wrap");
        check(!n::route(*t,0,n::count,*scratch).found,"invalid destination");
        Citizen c{};c.x=0;c.y=1;n::TravelProgress travel;
        check(n::advance(*t,c,travel,width+1,1)==n::TravelResult::waiting&&c.x==0&&travel.remaining==2,"wading consumes turns");
        check(n::advance(*t,c,travel,width+1,1)==n::TravelResult::already_used&&travel.remaining==2,"same tick cannot advance twice");
        check(n::advance(*t,c,travel,width+1,3)==n::TravelResult::waiting&&c.x==0,"interruption preserves progress");
        check(n::advance(*t,c,travel,width+1,4)==n::TravelResult::moved&&c.x==1,"cross only after full cost");
        check(n::advance(*t,c,travel,width+2,5)==n::TravelResult::waiting&&c.x==1,"exit ford consumes turns");
        check(n::advance(*t,c,travel,1,6)==n::TravelResult::blocked&&c.x==1&&!travel.remaining,"new invalid target cancels prior travel");
        std::cout<<"PASS terrain costs, path barriers, detours and real movement turns\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
