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
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto w=std::unique_ptr<World>(new World);initialize(*w,42);
        discovery::State knowledge{};build::State sites{};storage::State reserve{};
        cooking::State cooked{};shelter::State sheltered{};settlement::State home{};
        multi_field::State farms{};irrigation::State water{};Production p{};
        const auto all=static_cast<discovery::Mask>((1u<<discovery::tech_count)-1u);
        knowledge.known[0]=knowledge.known[1]=all;
        auto& a=w->founders[0];auto& b=w->founders[1];
        a.x=b.x=64;a.y=b.y=30;a.personality[0]=b.personality[0]=0;
        const auto base=static_cast<std::uint16_t>(30*width+50);
        sites.sites[build::hut].stage=build::Stage::complete;sites.sites[build::hut].tile=base;
        sites.sites[build::campfire].stage=build::Stage::complete;sites.sites[build::campfire].tile=base-1;
        sites.sites[build::store].stage=build::Stage::complete;sites.sites[build::store].tile=base-2;
        // Only one scarce, affordable plot is initially eligible. This is a
        // controlled planning test, not a claim about unforced world outcomes.
        for(auto& tile:w->tiles) {tile.food=70;tile.water=90;tile.wood=30;tile.stone=30;}
        auto& target=w->tiles[base+4];target.food=40;
        check(!build::can_start(*w,knowledge,sites,0,build::field),"remote founder cannot build here");
        const auto before=a.x;
        check(farm_planning::seek(*w,knowledge,sites,a)&&a.x==before-1&&a.action==Action::move,"intentional farm travel");
        a.x=64;a.action=Action::idle;a.hunger=12;
        check(!farm_planning::seek(*w,knowledge,sites,a),"urgent needs first");a.hunger=0;
        discovery::Event re{};build::Event ce{};
        for(unsigned i=0;i<240&&multi_field::completed(sites)==0;++i) {
            check(multi_field::step(*w,knowledge,sites,reserve,cooked,sheltered,home,farms,water,p,re,ce,discovery::probe_cadence),"real travel and build tick");
        }
        check(multi_field::completed(sites)==1&&sites.sites[build::field].tile==base+4,"actual planned field completion");
        // A displaced active builder must walk back rather than wait forever.
        auto& job=sites.sites[build::field];job.stage=build::Stage::building;job.owner=0;
        a.x=56;a.y=30;a.hunger=a.thirst=0;a.action=Action::idle;
        check(farm_planning::seek(*w,knowledge,sites,a)&&a.x==55,"interrupted project return");
        std::cout<<"PASS purposeful farm travel and real construction\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
