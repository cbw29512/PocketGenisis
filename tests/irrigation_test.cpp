#define PG_HOUSING_PROBE
#define PG_AGRICULTURE_PROBE
#define PG_IRRIGATION_PROBE
#include "../core/irrigation.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
namespace build=genesis::construction;
namespace irrig=genesis::irrigation;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto w=std::unique_ptr<World>(new World);initialize(*w,42);
        discovery::State research{};build::State sites{};irrig::State water{};
        auto& a=w->founders[0];auto& b=w->founders[1];w->tick=1;
        check(!discovery::eligible(0,discovery::irrigation)&&
              discovery::eligible(discovery::tech_bit(discovery::farming),discovery::irrigation),"knowledge prerequisites");
        const auto all=static_cast<discovery::Mask>((1u<<discovery::tech_count)-1u);
        auto& field=sites.sites[build::field];field.stage=build::Stage::complete;
        field.tile=build::location(a);
        sites.sites[build::hut].stage=build::Stage::complete;sites.sites[build::hut].tile=field.tile-1;
        sites.sites[build::campfire].stage=build::Stage::complete;sites.sites[build::campfire].tile=field.tile-2;
        sites.sites[build::store].stage=build::Stage::complete;sites.sites[build::store].tile=field.tile-3;
        ++a.x;b.x=a.x;b.y=a.y;
        auto& source=w->tiles[build::location(a)];auto& dest=w->tiles[field.tile];
        source.water=50;source.wood=20;source.stone=20;dest.water=0;
        check(!build::can_start(*w,research,sites,0,build::channel),"unknown irrigation");
        research.known[0]=research.known[1]=all;
        dest.water=24;check(!build::can_start(*w,research,sites,0,build::channel),"no water demand");
        dest.water=0;source.water=24;
        check(!build::can_start(*w,research,sites,0,build::channel),"source protected");
        source.water=50;++a.x;
        check(!build::can_start(*w,research,sites,0,build::channel),"adjacency");--a.x;
        auto e=build::advance(*w,research,sites);
        check(e.kind==build::EventKind::started&&e.type==build::channel&&source.wood==16&&source.stone==17,"construction debit");
        check(!irrig::transfer(*w,sites,water),"unfinished channel");
        for(unsigned i=0;i<30;++i){++w->tick;e=build::advance(*w,research,sites);}
        check(e.kind==build::EventKind::completed&&e.type==build::channel,"completed channel");
        w->tick=60;source.water=25;dest.water=0;
        check(irrig::transfer(*w,sites,water)==1&&source.water==24&&dest.water==1,"reserve conserved");
        check(!irrig::transfer(*w,sites,water),"no repeated transfer");
        w->tick=120;source.water=50;dest.water=23;
        check(irrig::transfer(*w,sites,water)==1&&source.water==49&&dest.water==24,"target cap");
        w->tick=180;check(!irrig::transfer(*w,sites,water),"no unnecessary transfer");
        dest.water=0;auto& channel=sites.sites[build::channel];const auto tile=channel.tile;
        channel.tile=field.tile+2;check(!irrig::transfer(*w,sites,water),"invalid link");
        channel.tile=tile;check(irrig::transfer(*w,sites,water)==2&&dest.water==2&&source.water==47&&irrig::valid(water),"full transfer accounting");
        // Real tick: natural renewal occurs first; irrigation only redistributes.
        w->tick=239;dest.food=0;dest.water=0;source.water=50;
        a.hunger=a.thirst=b.hunger=b.thirst=0;
        a.personality[0]=b.personality[0]=0;
        storage::State reserve{};cooking::State cooked{};shelter::State sheltered{};settlement::State home{};agriculture::State crops{};Production p{};
        // All natural stocks below cap here, so compute expected renewal from the real map.
        std::uint64_t expected=0;for(const auto& t:w->tiles) expected+=std::min(3u,100u-unsigned(t.water));
        discovery::Event re{};build::Event ce{};
        check(irrig::step(*w,research,sites,reserve,cooked,sheltered,home,crops,water,p,re,ce,discovery::probe_cadence),"real irrigation tick");
        check(dest.water==5&&source.water==51&&p.water==expected&&irrig::valid(water),"renewal versus transfer");
        initialize(*w,42);research={};sites={};water={};reserve={};cooked={};sheltered={};home={};crops={};
        for(unsigned i=0;i<43200;++i){
            check(irrig::step(*w,research,sites,reserve,cooked,sheltered,home,crops,water,p,re,ce,discovery::probe_cadence),"unforced tick");
            check(irrig::valid(water)&&agriculture::valid(crops)&&storage::valid(reserve),"integrated conservation");
        }
        check(valid_population(*w)&&discovery::discovered(research)>0&&build::completed(sites)>0,"integrated progress");
        std::cout<<"IRRIGATION_SMOKE discoveries="<<discovery::discovered(research)
                 <<" channels="<<(sites.sites[build::channel].stage==build::Stage::complete)
                 <<" transfers="<<water.transfers<<" moved_water="<<water.water<<" harvests="<<crops.harvests<<'\n';return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
