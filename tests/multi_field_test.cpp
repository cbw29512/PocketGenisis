#define PG_HOUSING_PROBE
#define PG_AGRICULTURE_PROBE
#define PG_IRRIGATION_PROBE
#define PG_MULTI_FIELD_PROBE
#include "../core/multi_field.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
namespace build=genesis::construction;
namespace farm=genesis::agriculture;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto w=std::unique_ptr<World>(new World);initialize(*w,42);
        discovery::State research{};build::State sites{};multi_field::State farms{};
        irrigation::State water{};storage::State reserve{};settlement::State home{};
        auto& a=w->founders[0];auto& b=w->founders[1];w->tick=1;
        research.known[0]=research.known[1]=static_cast<discovery::Mask>((1u<<discovery::tech_count)-1u);
        const auto first=build::field_slot(0),second=build::field_slot(1);
        const auto origin=build::location(a);
        sites.sites[first].stage=build::Stage::complete;sites.sites[first].tile=origin;
        sites.sites[build::hut].stage=build::Stage::complete;sites.sites[build::hut].tile=origin-1;
        sites.sites[build::campfire].stage=build::Stage::complete;sites.sites[build::campfire].tile=origin-2;
        sites.sites[build::store].stage=build::Stage::complete;sites.sites[build::store].tile=origin-3;
        a.x+=2;b.x=a.x;b.y=a.y;
        auto& stock=w->tiles[build::location(a)];stock.food=48;stock.water=50;stock.wood=20;stock.stone=20;
        check(!build::can_start(*w,research,sites,0,second),"no extra field demand");
        // Controlled larger family; production still uses actual living count.
        w->descendant_count=3;w->next_citizen_id=6;
        for(unsigned i=0;i<3;++i){
            auto& c=w->descendants[i];c.id=3+i;c.parent1=1;c.parent2=2;c.x=a.x;c.y=a.y;
        }
        check(build::can_start(*w,research,sites,0,second),"population and scarcity demand");
        stock.food=49;check(!build::can_start(*w,research,sites,0,second),"no local scarcity");stock.food=48;
        // Use the same construction engine; fill housing capacity to avoid competing demand.
        for(unsigned i=0;i<2;++i){
            auto& h=sites.sites[build::kind_count+i];h.stage=build::Stage::complete;h.tile=origin+10+i;
        }
        auto event=build::advance(*w,research,sites);
        check(event.kind==build::EventKind::started&&event.type==build::field&&stock.wood==18&&stock.stone==19,"shared field debit");
        for(unsigned i=0;i<20;++i){++w->tick;event=build::advance(*w,research,sites);}
        check(event.kind==build::EventKind::completed&&multi_field::completed(sites)==2,"second field completed");
        check(!build::can_start(*w,research,sites,0,build::field_slot(2)),"no excess expansion");
        auto& original=w->tiles[origin];original.food=48;original.water=50;
        check(farm::work(*w,research,sites,farms.crops[1],a,second)==farm::Task::plant,"second plot planted");
        check(farms.crops[0].stage==0&&original.food==48&&stock.food==46,"independent plot state and seed debit");
        check(!farm::Renewal{sites}(origin)&&!farm::Renewal{sites}(build::location(a))&&farm::Renewal{sites}(origin+1),"wild growth only outside fields");
        // Channel between both fields: choose driest, then stable slot order on tie.
        auto& ch=sites.sites[build::channel];ch.stage=build::Stage::complete;ch.tile=origin+1;
        auto& source=w->tiles[ch.tile];source.water=50;original.water=15;stock.water=0;w->tick=60;
        check(irrigation::transfer(*w,sites,water)==2&&stock.water==2&&original.water==15&&source.water==48,"channel serves second field");
        original.water=stock.water=2;w->tick=120;
        check(irrigation::transfer(*w,sites,water)==2&&original.water==4&&stock.water==2,"stable tie");
        // At tick 121, first field ready to plant, second not due yet.
        stock.water=50;original.water=50;w->tick=121;
        const multi_field::Work work{research,sites,reserve,farms,home,discovery::probe_cadence};
        check(work(*w,a)&&a.action==Action::move&&a.x==51,"walk to ready first field");
        a.x=50;a.action=Action::idle;
        const multi_field::Work local{research,sites,reserve,farms,home,discovery::probe_cadence};
        check(local(*w,a)&&local.chosen==a.id&&local.selected==0,"select independent ready field");
        check(farm::work(*w,research,sites,farms.crops[0],a,first)==farm::Task::plant&&multi_field::valid(farms),"both fields planted independently");
        initialize(*w,42);research={};sites={};farms={};water={};reserve={};home={};
        cooking::State cooked{};shelter::State sheltered{};Production p{};
        for(unsigned i=0;i<43200;++i){
            discovery::Event re{};build::Event ce{};
            check(multi_field::step(*w,research,sites,reserve,cooked,sheltered,home,farms,water,p,re,ce,discovery::probe_cadence),"integrated tick");
            check(multi_field::valid(farms)&&irrigation::valid(water)&&storage::valid(reserve),"accounting");
        }
        check(valid_population(*w)&&discovery::discovered(research)>0,"integrated world");
        std::cout<<"MULTI_FIELD_SMOKE fields="<<multi_field::completed(sites)<<" harvests="<<multi_field::harvests(farms)
                 <<" water="<<water.water<<" living="<<w->descendant_count+2<<'\n';return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
