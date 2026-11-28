#define PG_HOUSING_PROBE
#define PG_AGRICULTURE_PROBE
#define PG_IRRIGATION_PROBE
#define PG_MULTI_FIELD_PROBE
#define PG_CROP_LOSS_PROBE
#include "../core/crop_loss.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
namespace build=genesis::construction;
namespace farm=genesis::agriculture;
namespace loss=genesis::crop_loss;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto w=std::unique_ptr<World>(new World);initialize(*w,42);
        discovery::State knowledge{};build::State sites{};multi_field::State farms{};
        irrigation::State water{};auto& a=w->founders[0];auto& crop=farms.crops[0];
        const auto all=static_cast<discovery::Mask>((1u<<discovery::tech_count)-1u);
        knowledge.known[0]=knowledge.known[1]=all;
        auto& field=sites.sites[build::field];field.stage=build::Stage::complete;field.tile=build::location(a);
        auto& stock=w->tiles[field.tile];stock.food=50;stock.water=50;w->tick=1;
        check(farm::work(*w,knowledge,sites,crop,a)==farm::Task::plant,"real seeds");
        stock.water=0;w->tick=2;
        check(loss::evaluate(*w,sites,crop,build::field)==loss::Cause::none&&crop.dry_since==2,"start dry period");
        w->tick=601;check(loss::evaluate(*w,sites,crop,build::field)==loss::Cause::none,"drought grace");
        // Irrigation before risk evaluation interrupts the actual dry period.
        auto& channel=sites.sites[build::channel];channel.stage=build::Stage::complete;channel.tile=field.tile+1;
        auto& source=w->tiles[channel.tile];source.water=50;w->tick=660;
        check(irrigation::transfer(*w,sites,water)==2&&source.water==48&&stock.water==2,"real irrigation");
        check(loss::evaluate(*w,sites,crop,build::field)==loss::Cause::none&&!crop.dry_since&&crop.stage==1,"water prevents loss");
        stock.water=0;w->tick=661;loss::evaluate(*w,sites,crop,build::field);
        w->tick=1261;
        check(loss::evaluate(*w,sites,crop,build::field)==loss::Cause::drought&&
              crop.stage==0&&crop.drought_losses==1&&crop.lost_water==1&&stock.food==48&&farm::valid(crop),"drought loss accounting");
        check(loss::evaluate(*w,sites,crop,build::field)==loss::Cause::none&&crop.drought_losses==1,"no duplicate loss");
        stock.water=50;w->tick=1262;
        check(farm::work(*w,knowledge,sites,crop,a)==farm::Task::plant,"replant after loss");
        for(unsigned i=0;i<3;++i){w->tick+=farm::work_interval;check(farm::work(*w,knowledge,sites,crop,a)==farm::Task::tend,"real tending");}
        w->tick+=farm::work_interval+loss::ripe_limit-1;
        check(loss::evaluate(*w,sites,crop,build::field)==loss::Cause::none,"ripe grace");
        // Independent second crop must not be changed by a first-field loss.
        const auto second=build::field_slot(1);sites.sites[second].stage=build::Stage::complete;sites.sites[second].tile=field.tile+2;
        a.x+=2;auto& other=w->tiles[sites.sites[second].tile];other.food=50;other.water=50;
        check(farm::work(*w,knowledge,sites,farms.crops[1],a,second)==farm::Task::plant,"independent crop");
        a.x-=2;++w->tick;
        check(loss::evaluate(*w,sites,crop,build::field)==loss::Cause::spoilage&&
              crop.spoilage_losses==1&&crop.lost_water==5&&stock.food==46&&
              farms.crops[1].stage==1&&multi_field::valid(farms),"spoilage and independent plot accounting");
        // Winter pauses immature drought evaluation, consistent with growth rules.
        w->tick=3*ticks_per_year/4;other.water=0;
        check(loss::evaluate(*w,sites,farms.crops[1],second)==loss::Cause::none&&!farms.crops[1].dry_since,"winter dormancy");
        // Natural harvest after losses must still account correctly.
        w->tick=ticks_per_year;stock.water=50;
        check(farm::work(*w,knowledge,sites,crop,a)==farm::Task::plant,"new year planting");
        for(unsigned i=0;i<3;++i){w->tick+=farm::work_interval;check(farm::work(*w,knowledge,sites,crop,a)==farm::Task::tend,"new crop tending");}
        w->tick+=farm::work_interval;
        check(farm::work(*w,knowledge,sites,crop,a)==farm::Task::harvest&&crop.harvests==1&&crop.food==8&&farm::valid(crop),"harvest after losses");
        initialize(*w,42);knowledge={};sites={};farms={};water={};
        storage::State reserve{};cooking::State cooked{};shelter::State sheltered{};settlement::State home{};Production p{};
        for(unsigned i=0;i<43200;++i){
            discovery::Event re{};build::Event ce{};
            check(loss::step(*w,knowledge,sites,reserve,cooked,sheltered,home,farms,water,p,re,ce,discovery::probe_cadence),"unforced tick");
            check(multi_field::valid(farms)&&storage::valid(reserve)&&irrigation::valid(water),"integrated accounting");
        }
        std::uint64_t dry=0,spoiled=0;
        for(const auto& c:farms.crops){dry+=c.drought_losses;spoiled+=c.spoilage_losses;}
        check(valid_population(*w)&&discovery::discovered(knowledge)>0,"integrated population");
        std::cout<<"CROP_LOSS_SMOKE fields="<<multi_field::completed(sites)<<" harvests="<<multi_field::harvests(farms)
                 <<" drought="<<dry<<" spoilage="<<spoiled<<" living="<<w->descendant_count+2<<'\n';
        return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
