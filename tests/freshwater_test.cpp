#define PG_HOUSING_PROBE
#define PG_AGRICULTURE_PROBE
#define PG_IRRIGATION_PROBE
#define PG_MULTI_FIELD_PROBE
#define PG_CROP_LOSS_PROBE
#define PG_LIVESTOCK_PROBE
#include "../core/terrain_renewal.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
namespace fw=genesis::freshwater;
namespace tp=genesis::terrain_probe;
namespace tr=genesis::terrain_resources;
namespace build=genesis::construction;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto w=std::make_unique<World>();auto terrain=std::make_unique<tp::State>();
        auto fish=std::make_unique<fw::State>();auto wild=std::make_unique<wildlife::Wildlife>();
        initialize(*w,42);auto& a=w->founders[0];const auto i=build::location(a);
        for(auto& c:terrain->cells)c={120,100,tp::Biome::grassland};
        terrain->cells[i]={85,144,tp::Biome::lowland};
        fw::initialize(*fish,*terrain);
        check(fish->initial==2&&fw::valid(*fish)&&!fw::available(*fish,i+1),"actual habitat only");
        auto& stock=w->tiles[i];stock.food=1;stock.water=100;
        discovery::State knowledge{};build::State sites{};storage::State reserve{};
        cooking::State cooked{};shelter::State sheltered{};settlement::State home{};
        multi_field::State farms{};irrigation::State channels{};livestock::State animals{};
        wildlife::Totals ecology{};Production p{};discovery::Event re{};build::Event ce{};
        a.hunger=11;a.thirst=0;
        const terrain_renewal::Policy policy{*terrain,{sites},*wild};
        check(livestock::step_with_renewal(*w,knowledge,sites,reserve,cooked,sheltered,home,
              farms,channels,*wild,ecology,animals,p,re,ce,discovery::probe_cadence,policy,fish.get()),"integrated fish meal");
        check(a.action==Action::eat&&a.hunger==0&&stock.food==1&&fish->caught==1&&
              fw::population(*fish)==1&&!ecology.meals&&fw::valid(*fish),"one actual fish, no double meal");
        check(!fw::catch_one(*fish,i+1)&&fw::catch_one(*fish,i)&&!fw::catch_one(*fish,i),"finite local fish");
        w->tick=3600;fw::pulse(*w,*fish);
        check(!fw::population(*fish)&&fw::valid(*fish),"no extinction resurrection");
        fw::initialize(*fish,*terrain);fw::pulse(*w,*fish);
        check(fish->births==1&&fw::population(*fish)==3&&fw::valid(*fish),"resource limited reproduction");
        const auto consumed=fish->forage_eaten;fw::pulse(*w,*fish);
        check(fish->forage_eaten==consumed,"duplicate pulse blocked");
        stock.water=0;w->tick+=600;fw::pulse(*w,*fish);
        check(fish->deaths==3&&!fw::population(*fish)&&fw::valid(*fish),"habitat drought mortality");
        // Controlled winter comparison: identical habitats and fish,
        // distinct feeding policy, no new stock or fish injected midrun.
        auto baseline=std::make_unique<fw::State>();
        terrain->cells[i].moisture=145;
        fw::initialize(*fish,*terrain);
        *baseline=*fish;
        stock.water=100;
        for(unsigned n=0;n<18;++n) {
            w->tick=32400+600*n;
            fw::pulse(*w,*baseline);
            fw::pulse(*w,*fish,true);
        }
        check(fw::valid(*baseline)&&fw::valid(*fish)&&
              fw::population(*fish)==3&&fw::population(*baseline)<3&&
              !fish->births,"cold-season feeding preserves actual fish without winter births");
        // Local refuge: a threatened group may move only through adjacent
        // freshwater cells, and only once in the checkerboard pulse.
        terrain->cells[i].moisture=144;
        terrain->cells[i+1]={85,144,tp::Biome::lowland};
        fw::initialize(*fish,*terrain);
        w->tiles[i].water=0;w->tiles[i+1].water=100;
        w->tick=1200; // founder tile (50,30) has even checkerboard parity
        fw::pulse(*w,*fish,true);
        check(fish->migrated==2&&fish->cells[i].fish==0&&
              fish->cells[i+1].fish==4&&fw::valid(*fish),"actual local refuge transfer");
        w->tiles[i+1].water=0;w->tick=1800;
        fw::pulse(*w,*fish,true);
        check(!fw::population(*fish)&&fish->water_deaths==4&&
              fw::valid(*fish),"no refuge means actual water mortality");
        // Paired natural runs with identical geography and weather,
        // including the old seed42 baseline and independent seed0.
        for(bool balanced:{false,true}) for(auto seed:{0ULL,42ULL}) {
        initialize(*w,seed);tp::generate(*terrain,seed);tr::initialize_stocks(*w,*terrain);
        fw::initialize(*fish,*terrain);wildlife::initialize(*wild,seed);
        knowledge={};sites={};reserve={};cooked={};sheltered={};home={};farms={};
        channels={};animals={};ecology={};
        const auto initial_wild=wildlife::population(*wild);
        regional_weather::Cache climate(seed);
        const terrain_renewal::Policy current{*terrain,{sites},*wild,&climate};
        for(unsigned n=0;n<43200;++n)
            check(livestock::step_with_renewal(*w,knowledge,sites,reserve,cooked,sheltered,home,
                  farms,channels,*wild,ecology,animals,p,re,ce,discovery::probe_cadence,current,fish.get(),balanced),"natural aquatic tick");
        check(fw::valid(*fish)&&valid_population(*w)&&livestock::valid(animals)&&
              multi_field::valid(farms)&&wildlife::valid(*wild),"integrated ecosystem accounting");
        check(initial_wild+ecology.births==wildlife::population(*wild)+ecology.deaths+
              livestock::captures(animals),"wild accounting separate from fish");
        std::uint64_t meat=0;for(const auto& t:wild->tiles) meat+=t.meat;
        check(ecology.meat_created+animals.meat_produced==meat+ecology.meals+ecology.spoiled,"meat accounting");
        std::cout<<"FRESHWATER_ECOSYSTEM seed="<<seed<<" balanced="<<balanced
                 <<" initial="<<fish->initial<<" births="<<fish->births
                 <<" deaths="<<fish->deaths<<" water_deaths="<<fish->water_deaths
                 <<" food_deaths="<<fish->food_deaths<<" migrated="<<fish->migrated
                 <<" caught="<<fish->caught<<" surviving="<<fw::population(*fish)
                 <<" wild="<<wildlife::population(*wild)<<" crops="<<multi_field::harvests(farms)
                 <<" managed="<<livestock::population(animals)<<" living="<<w->descendant_count+2<<'\n';
        }
        std::cout<<"PASS freshwater habitat, fish conservation and integrated fishing\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
