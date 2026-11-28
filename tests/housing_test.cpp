#define PG_HOUSING_PROBE
#include "../core/settlement.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
namespace build=genesis::construction;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto w=std::unique_ptr<World>(new World);initialize(*w,42);
        discovery::State research{};build::State sites{};storage::State reserve{};
        cooking::State cooked{};shelter::State sheltered{};settlement::State home{};
        auto& a=w->founders[0];auto& b=w->founders[1];w->tick=1;
        const auto all=static_cast<discovery::Mask>((1u<<discovery::tech_count)-1u);
        research.known[0]=research.known[1]=all;
        sites.sites[build::hut].stage=build::Stage::complete;sites.sites[build::hut].tile=build::location(a);
        sites.sites[build::campfire].stage=build::Stage::complete;sites.sites[build::campfire].tile=build::location(a)-1;
        sites.sites[build::store].stage=build::Stage::complete;sites.sites[build::store].tile=build::location(a)-2;
        ++a.x;++a.x;auto& stock=w->tiles[build::location(a)];stock.food=stock.water=50;stock.wood=stock.stone=50;
        check(!build::housing_demand(*w,sites)&&!build::can_start(*w,research,sites,0,build::kind_count),"no demand");
        // One existing child creates real demand; no synthetic birth in integrated run.
        w->descendant_count=1;w->next_citizen_id=4;auto& child=w->descendants[0];
        child.id=3;child.parent1=1;child.parent2=2;child.x=b.x;child.y=b.y;
        check(build::housing_demand(*w,sites)==1,"living demand");
        stock.food=0;check(!build::can_start(*w,research,sites,0,build::kind_count),"food viability");
        stock.food=50;stock.wood=0;check(!build::can_start(*w,research,sites,0,build::kind_count),"materials");
        stock.wood=50;a.x+=9;check(!build::can_start(*w,research,sites,0,build::kind_count),"remote expansion");a.x-=9;
        auto e=build::advance(*w,research,sites);
        check(e.kind==build::EventKind::started&&e.type==build::hut&&stock.wood==42&&stock.stone==46,"shared blueprint debit");
        check(build::active(sites)==int(build::kind_count),"extra site active");
        a.hunger=12;++w->tick;build::advance(*w,research,sites);
        check(sites.sites[build::kind_count].work==0,"survival first");
        a.hunger=0;
        for(unsigned i=0;i<40;++i){++w->tick;e=build::advance(*w,research,sites);}
        check(e.kind==build::EventKind::completed&&e.type==build::hut&&build::completed_huts(sites)==2,"extra hut completed");
        check(!build::housing_demand(*w,sites)&&build::advance(*w,research,sites).kind==build::EventKind::none,"stop when demand met");
        check(build::occupied(sites,build::location(a)),"extra site occupancy");
        a.health=80;++a.x;check(settlement::return_home(*w,sites,a,home)&&home.arrivals==1,"nearest extra shelter");
        a.action=Action::idle;a.health=50;b.health=50;b.x=sites.sites[build::hut].tile%width;b.y=sites.sites[build::hut].tile/width;
        child.health=50;child.x=b.x;child.y=b.y;update_mortality(*w,shelter::Recovery{sites,sheltered});
        check(a.health==52&&b.health==52&&child.health==52&&sheltered.extra_recovery==3,"independent shelter recovery slots");
        initialize(*w,42);research={};sites={};reserve={};cooked={};sheltered={};home={};Production p{};
        unsigned births=0;
        for(unsigned i=0;i<43200;++i){
            discovery::Event re{};build::Event ce{};
            check(settlement::step(*w,research,sites,reserve,cooked,sheltered,home,p,re,ce,discovery::probe_cadence),"integrated tick");
            check(storage::valid(reserve)&&cooking::valid(cooked)&&settlement::valid(home),"accounting");
            for(std::size_t j=0;j<w->descendant_count;++j) births+=w->descendants[j].birth_tick==w->tick;
        }
        check(build::completed_huts(sites)>0&&births>0&&valid_population(*w),"integrated housing and natural births");
        std::cout<<"HOUSING_SMOKE huts="<<build::completed_huts(sites)<<" demand="<<build::housing_demand(*w,sites)<<" births="<<births<<'\n';
        return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
