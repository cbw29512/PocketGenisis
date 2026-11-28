#define PG_HOUSING_PROBE
#define PG_AGRICULTURE_PROBE
#define PG_IRRIGATION_PROBE
#define PG_MULTI_FIELD_PROBE
#define PG_CROP_LOSS_PROBE
#define PG_LIVESTOCK_PROBE
#define PG_SOIL_FERTILITY_PROBE
#include "../core/terrain_renewal.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
namespace farm=genesis::agriculture;
namespace build=genesis::construction;
namespace tp=genesis::terrain_probe;
namespace tr=genesis::terrain_resources;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto w=std::make_unique<World>();initialize(*w,42);w->tick=1;
        discovery::State knowledge{};build::State sites{};multi_field::State farms{};
        auto& a=w->founders[0];auto& crop=farms.crops[0];
        knowledge.known[0]=discovery::tech_bit(discovery::farming);
        const auto tile=build::location(a);
        sites.sites[build::field].stage=build::Stage::complete;sites.sites[build::field].tile=tile;
        auto& stock=w->tiles[tile];stock.food=50;stock.water=100;
        unsigned first=0,last=0;
        for(unsigned n=0;n<10;++n) {
            check(farm::work(*w,knowledge,sites,crop,a)==farm::Task::plant,"plant actual seed and nutrients");
            for(unsigned j=0;j<3;++j) {
                w->tick+=farm::work_interval;
                check(farm::work(*w,knowledge,sites,crop,a)==farm::Task::tend,"real tending");
            }
            w->tick+=farm::work_interval;
            const auto before=stock.food;
            check(farm::work(*w,knowledge,sites,crop,a)==farm::Task::harvest,"actual variable harvest");
            last=stock.food-before;if(!n) first=last;
            check(farm::valid(crop),"crop and nutrient accounting");
            for(unsigned j=0;j<last;++j) {
                a.hunger=12;a.action=Action::idle;
                check(consume_resource(*w,a,a.x,a.y,false),"consume harvested stock");
            }
            a.action=Action::idle;
        }
        check(first==8&&last<first&&crop.nutrients.fertility==20&&
              farm::task(*w,sites,crop,w->tick)==farm::Task::none,"exhausted soil blocks planting");
        const auto previous_food=stock.food;
        const auto before=crop.nutrients.restored;
        w->tick=6000;stock.water=0;
        farm::recover_soil(*w,sites,crop,build::field);
        check(crop.nutrients.restored==before,"dry field cannot recover");
        stock.water=60;
        for(unsigned n=0;n<4;++n) {
            w->tick=6600+600*n;
            farm::recover_soil(*w,sites,crop,build::field);
            farm::recover_soil(*w,sites,crop,build::field);
        }
        check(crop.nutrients.fertility==24&&crop.nutrients.restored==before+4&&
              stock.food==previous_food&&farm::valid(crop),"gradual moist fallow recovery without invented food");
        check(farm::work(*w,knowledge,sites,crop,a)==farm::Task::plant,"recovered soil permits planting");
        const auto rest=crop.nutrients.restored;w->tick=9000;
        farm::recover_soil(*w,sites,crop,build::field);
        check(crop.nutrients.restored==rest,"no recovery while cultivated");
        // Actual new world with terrain, weather, wildlife and fishing.
        initialize(*w,42);knowledge={};sites={};farms={};
        auto terrain=std::make_unique<tp::State>();tp::generate(*terrain,42);
        tr::initialize_stocks(*w,*terrain);
        auto wild=std::make_unique<wildlife::Wildlife>();wildlife::initialize(*wild,42);
        auto fish=std::make_unique<freshwater::State>();freshwater::initialize(*fish,*terrain);
        regional_weather::Cache climate(42);
        const terrain_renewal::Policy policy{*terrain,{sites},*wild,&climate};
        storage::State reserve{};cooking::State cooked{};shelter::State sheltered{};
        settlement::State home{};irrigation::State channels{};livestock::State animals{};
        wildlife::Totals ecology{};Production p{};discovery::Event re{};build::Event ce{};
        for(unsigned n=0;n<43200;++n) {
            check(livestock::step_with_renewal(*w,knowledge,sites,reserve,cooked,sheltered,
                  home,farms,channels,*wild,ecology,animals,p,re,ce,discovery::probe_cadence,
                  policy,fish.get(),true),"integrated soil tick");
            check(multi_field::valid(farms),"actual crop accounting");
        }
        check(valid_population(*w)&&freshwater::valid(*fish)&&livestock::valid(animals),"integrated ecosystem validity");
        std::cout<<"SOIL_ECOSYSTEM harvests="<<multi_field::harvests(farms)
                 <<" field0_fertility="<<unsigned(farms.crops[0].nutrients.fertility)
                 <<" spent="<<farms.crops[0].nutrients.spent
                 <<" recovered="<<farms.crops[0].nutrients.restored
                 <<" food="<<farms.crops[0].food<<" living="<<w->descendant_count+2
                 <<" managed="<<livestock::population(animals)<<" fish="<<freshwater::population(*fish)<<'\n';
        std::cout<<"PASS soil depletion, variable yield and conditional fallow recovery\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
