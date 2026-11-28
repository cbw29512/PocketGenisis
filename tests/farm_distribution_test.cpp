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
        auto w=std::make_unique<World>();auto wild=std::make_unique<wildlife::Wildlife>();
        initialize(*w,42);w->tick=1;
        discovery::State knowledge{};build::State sites{};multi_field::State farms{};
        storage::State reserve{};settlement::State home{};livestock::State animals{};
        auto& a=w->founders[0];
        const auto all=static_cast<discovery::Mask>((1u<<discovery::tech_count)-1u);
        knowledge.known[0]=knowledge.known[1]=all;
        // Close unrelated construction opportunities in this controlled fixture.
        for(unsigned k=0;k<build::site_count;++k) {
            sites.sites[k].stage=build::Stage::abandoned;sites.sites[k].tile=static_cast<std::uint16_t>(k);
        }
        const auto tile=build::location(a);
        sites.sites[build::field].stage=build::Stage::complete;sites.sites[build::field].tile=tile;
        for(auto& t:w->tiles){t.food=100;t.water=100;}
        auto& stock=w->tiles[tile];stock.food=98;auto& crop=farms.crops[0];
        check(farm::task(*w,sites,crop,w->tick)==farm::Task::none,"no false demand");
        w->tiles[tile+1].food=40;w->tiles[tile-1].food=20;
        check(farm::task(*w,sites,crop,w->tick)==farm::Task::distribute,"real overflow demand");
        check(farm::work(*w,knowledge,sites,crop,a)==farm::Task::distribute&&
              stock.food==94&&w->tiles[tile-1].food==24&&crop.distributed==4&&crop.last_work==0,"first conserved handoff");
        check(farm::work(*w,knowledge,sites,crop,a)==farm::Task::distribute&&
              stock.food==90&&w->tiles[tile-1].food==28&&crop.distributed==8&&farm::valid(crop),"second conserved handoff");
        check(farm::work(*w,knowledge,sites,crop,a)==farm::Task::plant&&stock.food==88&&crop.stage==1,"real planting after storage cleared");
        for(unsigned i=0;i<3;++i){w->tick+=farm::work_interval;check(farm::work(*w,knowledge,sites,crop,a)==farm::Task::tend,"actual tending");}
        w->tick+=farm::work_interval;
        check(farm::work(*w,knowledge,sites,crop,a)==farm::Task::harvest&&stock.food==96&&crop.harvests==1&&farm::valid(crop),"real harvest");
        // Travel toward a known farm from well outside the old eight-tile radius.
        a.x+=20;a.action=Action::idle;a.hunger=a.thirst=0;
        const multi_field::Work policy{knowledge,sites,reserve,farms,home,discovery::probe_cadence};
        check(policy.productive(*w,a)&&a.x==69&&a.action==Action::move,"longer ordinary farm return");
        check(stock.food==96&&crop.distributed==8,"travel cannot transfer food");
        a.action=Action::idle;a.hunger=12;
        check(!policy.productive(*w,a),"urgent needs block return work");
        // Known enclosure and local animals; distant travel is not remote capture.
        const auto pen=tile+2;
        sites.sites[build::enclosure].stage=build::Stage::complete;sites.sites[build::enclosure].tile=pen;
        livestock::mark_development(*w,sites,*wild);
        wild->tiles[pen+1].count[wildlife::chicken]=2;
        a.hunger=0;a.action=Action::idle;
        check(livestock::seek_work(*w,knowledge,sites,*wild,animals,a)&&a.x==68,"longer ordinary livestock return");
        check(!livestock::population(animals)&&wild->tiles[pen+1].count[wildlife::chicken]==2,"no remote capture");
        std::cout<<"PASS farm surplus distribution and known workplace return\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
