#define PG_HOUSING_PROBE
#define PG_AGRICULTURE_PROBE
#define PG_IRRIGATION_PROBE
#define PG_MULTI_FIELD_PROBE
#define PG_CROP_LOSS_PROBE
#define PG_LIVESTOCK_PROBE
#include "../core/livestock_integration.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
namespace build=genesis::construction;
namespace farm=genesis::agriculture;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto w=std::make_unique<World>();
        initialize(*w,42);w->tick=1;
        discovery::State knowledge{};build::State sites{};multi_field::State farms{};
        storage::State reserve{};settlement::State home{};
        auto& a=w->founders[0];
        const auto all=static_cast<discovery::Mask>((1u<<discovery::tech_count)-1u);
        knowledge.known[0]=knowledge.known[1]=all;
        for(unsigned k=0;k<build::site_count;++k) {
            sites.sites[k].stage=build::Stage::abandoned;sites.sites[k].tile=static_cast<std::uint16_t>(k);
        }
        const auto tile=build::location(a);
        sites.sites[build::field].stage=build::Stage::complete;sites.sites[build::field].tile=tile;
        for(auto& t:w->tiles){t.food=100;t.water=100;}
        auto& stock=w->tiles[tile];auto& crop=farms.crops[0];
        check(farm::work(*w,knowledge,sites,crop,a)==farm::Task::plant&&stock.food==98,"full field plant uses real seed");
        for(unsigned i=0;i<3;++i) {
            w->tick+=farm::work_interval;
            check(farm::work(*w,knowledge,sites,crop,a)==farm::Task::tend,"actual growing work");
        }
        w->tick+=farm::work_interval;
        check(farm::task(*w,sites,crop,w->tick)==farm::Task::none&&crop.stage==4&&stock.food==98,"full harvest correctly blocked");
        check(farm::viable_tending(*w,sites,crop,w->tick),"ripe plot remains viable");
        // Actual human consumption frees room; not an arbitrary stock deletion.
        for(unsigned i=0;i<6;++i) {
            a.hunger=12;a.action=Action::idle;
            check(consume_resource(*w,a,a.x,a.y,false),"real consumption frees stock");
        }
        a.action=Action::idle;
        check(stock.food==92&&farm::work(*w,knowledge,sites,crop,a)==farm::Task::harvest&&stock.food==100&&farm::valid(crop),"full bounded harvest");
        // Check that a distant known ripe plot can attract a worker when
        // ready work is blocked by storage, without inventing a harvest.
        check(farm::work(*w,knowledge,sites,crop,a)==farm::Task::plant,"second crop");
        for(unsigned i=0;i<3;++i) {
            w->tick+=farm::work_interval;
            check(farm::work(*w,knowledge,sites,crop,a)==farm::Task::tend,"second growing work");
        }
        w->tick+=farm::work_interval;
        a.x+=20;a.action=Action::idle;a.hunger=a.thirst=0;
        const multi_field::Work policy{knowledge,sites,reserve,farms,home,discovery::probe_cadence};
        check(policy.locality(*w,a)&&a.x==69&&a.action==Action::move,"known blocked ripe crop return");
        check(crop.harvests==1&&stock.food==98,"travel has no production");
        // Exercise the actual simulation, not only direct work calls. Keep
        // other construction closed and let real needs free crop capacity.
        initialize(*w,42);knowledge={};knowledge.known[0]=knowledge.known[1]=all;
        farms={};home={};reserve={};
        for(auto& t:w->tiles){t.food=100;t.water=100;}
        cooking::State cooked{};shelter::State sheltered{};irrigation::State water{};
        Production p{};
        for(unsigned i=0;i<2400;++i) {
            discovery::Event re{};build::Event ce{};
            check(multi_field::step(*w,knowledge,sites,reserve,cooked,sheltered,home,farms,water,p,re,ce,discovery::probe_cadence),"authoritative crop tick");
            check(multi_field::valid(farms),"crop accounting");
        }
        check(multi_field::harvests(farms)>0,"real integrated harvest from full initial field");
        std::cout<<"FULL_FIELD_SMOKE harvests="<<multi_field::harvests(farms)
                 <<" distributed="<<farms.crops[0].distributed<<'\n';return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
