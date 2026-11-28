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
namespace tp=genesis::terrain_probe;
namespace tr=genesis::terrain_resources;
namespace renewal=genesis::terrain_renewal;
namespace build=genesis::construction;
namespace herd=genesis::livestock;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto w=std::make_unique<World>();auto terrain=std::make_unique<tp::State>();
        auto deposits=std::make_unique<tr::State>();auto wild=std::make_unique<wildlife::Wildlife>();
        initialize(*w,42);
        build::State sites{};
        constexpr std::size_t forest=0,dry=1,field=2,built=3;
        terrain->cells[forest].biome=tp::Biome::forest;
        terrain->cells[dry].biome=tp::Biome::dryland;
        terrain->cells[field].biome=tp::Biome::forest;
        terrain->cells[built].biome=tp::Biome::forest;
        sites.sites[build::field].stage=build::Stage::complete;
        sites.sites[build::field].tile=field;
        wild->developed[built]=1;
        const renewal::Policy policy{*terrain,{sites},*wild};
        unsigned growth=0;
        for(unsigned i=1;i<=100;++i) growth+=renewal::growth(1,15,60*i,dry);
        check(growth==15,"fractional growth conserved over complete cycle");
        // Four empty controlled stocks recover according to actual biome and
        // land-use policy. Minerals and stone cannot regenerate.
        Production total{};
        for(auto i:{forest,dry,field,built}) w->tiles[i]={0,0,0,7};
        for(unsigned n=1;n<=100;++n)
            for(auto i:{forest,dry,field,built})
                renewal::renew_tile(w->tiles[i],total,i,n*60,2,3,1,policy);
        check(w->tiles[forest].wood==100&&w->tiles[dry].wood==15&&
              w->tiles[forest].food==90&&w->tiles[dry].food==35&&
              w->tiles[dry].water==30,"persistent natural stock differences");
        check(w->tiles[field].food==0&&w->tiles[built].wood==0,"crop and development suppression");
        std::uint64_t food=0,water=0,wood=0;
        for(auto i:{forest,dry,field,built}) {
            const auto& t=w->tiles[i];food+=t.food;water+=t.water;wood+=t.wood;
            check(t.stone==7,"finite stone unchanged");
        }
        check(total.food==food&&total.water==water&&total.wood==wood,"actual renewal totals");
        w->tiles[dry].food=80;w->tiles[dry].wood=60;
        Production winter{};
        renewal::renew_tile(w->tiles[dry],winter,dry,32400,0,2,0,policy);
        check(w->tiles[dry].food==80&&w->tiles[dry].wood==60&&!winter.food&&!winter.wood,"winter and above-cap stock preserved");
        // Weather cache is derived from seed and epoch, never from live
        // decision RNG. Rebuilding after a reload gives the same region.
        namespace weather=genesis::regional_weather;
        weather::Cache climate(42),reloaded(42);
        unsigned changed=0;std::array<unsigned,4> occurrences{};
        for(std::uint64_t tick=0;tick<ticks_per_year;tick+=weather::interval) {
            climate.refresh(tick);reloaded.refresh(tick);
            for(unsigned r=0;r<weather::columns*weather::rows;++r) {
                check(climate.regions[r]==reloaded.regions[r],"weather cache replay");
                ++occurrences[static_cast<unsigned>(climate.regions[r])];
            }
            check(climate.at(tick,0)==climate.at(tick,9)&&
                  climate.at(tick,0)==climate.at(tick,9*width),"coherent local weather");
            const auto prior=climate.regions[0];
            changed+=prior!=climate.at(tick+weather::interval,0);
        }
        check(changed>0&&occurrences[0]&&occurrences[1]&&occurrences[2]&&occurrences[3],"varying regional weather");
        // Controlled identical empty forest stocks during one weather pulse.
        // Seed the derived cache explicitly only in this fixture.
        weather::Cache dry_weather(0),wet_weather(0);
        dry_weather.epoch=wet_weather.epoch=0;
        dry_weather.regions.fill(weather::Kind::drought);
        wet_weather.regions.fill(weather::Kind::rain);
        const renewal::Policy dry_policy{*terrain,{sites},*wild,&dry_weather};
        const renewal::Policy wet_policy{*terrain,{sites},*wild,&wet_weather};
        Tile dry_stock{},wet_stock{};Production dry_total{},wet_total{};
        for(unsigned n=1;n<30;++n) {
            renewal::renew_tile(dry_stock,dry_total,forest,n*60,2,3,1,dry_policy);
            renewal::renew_tile(wet_stock,wet_total,forest,n*60,2,3,1,wet_policy);
        }
        check(wet_total.food>dry_total.food&&wet_total.water>dry_total.water&&wet_total.wood>dry_total.wood,
              "rain and drought change actual resource production");
        check(dry_total.food==dry_stock.food&&wet_total.water==wet_stock.water,"weather resource conservation");
        // Preserve both terrain-only baselines, and run the same seeds
        // with regional weather enabled, without injected technology or food.
        for(bool weather_enabled:{false,true}) for(auto seed:{0ULL,42ULL}) {
            initialize(*w,seed);tp::generate(*terrain,seed);
            tr::generate(*deposits,*terrain,seed);tr::initialize_stocks(*w,*terrain);
            wildlife::initialize(*wild,seed);sites={};
            const auto initial_wild=wildlife::population(*wild);
            discovery::State knowledge{};storage::State reserve{};cooking::State cooked{};
            shelter::State sheltered{};settlement::State home{};multi_field::State farms{};
            irrigation::State channels{};herd::State animals{};wildlife::Totals ecology{};
            Production p{},year{};discovery::Event re{};build::Event ce{};
            const auto initial_terrain=tp::fingerprint(*terrain);
            weather::Cache regional(seed);
            const renewal::Policy current{*terrain,{sites},*wild,weather_enabled?&regional:nullptr};
            for(unsigned n=0;n<43200;++n) {
                check(herd::step_with_renewal(*w,knowledge,sites,reserve,cooked,sheltered,
                      home,farms,channels,*wild,ecology,animals,p,re,ce,discovery::probe_cadence,current),"integrated terrain tick");
                year.food+=p.food;year.water+=p.water;year.wood+=p.wood;
                check(herd::valid(animals)&&multi_field::valid(farms)&&storage::valid(reserve),"local accounting");
            }
            check(valid_population(*w)&&wildlife::valid(*wild)&&tr::valid(*deposits)&&
                  tp::fingerprint(*terrain)==initial_terrain,"valid final immutable geography");
            check(initial_wild+ecology.births==wildlife::population(*wild)+ecology.deaths+herd::captures(animals),"wild population conservation");
            std::uint64_t meat=0;for(const auto& tile:wild->tiles) meat+=tile.meat;
            check(ecology.meat_created+animals.meat_produced==meat+ecology.meals+ecology.spoiled,"meat accounting");
            std::cout<<"TERRAIN_ECOSYSTEM seed="<<seed<<" weather="<<weather_enabled<<" food="<<year.food<<" water="<<year.water
                     <<" wood="<<year.wood<<" wild="<<wildlife::population(*wild)
                     <<" hunted="<<ecology.hunted<<" meals="<<ecology.meals
                     <<" harvests="<<multi_field::harvests(farms)<<" managed="<<herd::population(animals)
                     <<" captures="<<herd::captures(animals)<<" discoveries="<<discovery::discovered(knowledge)
                     <<" living="<<w->descendant_count+2<<" health1="<<unsigned(w->founders[0].health)
                     <<" health2="<<unsigned(w->founders[1].health)<<'\n';
        }
        std::cout<<"PASS terrain-dependent renewal and integrated ecology accounting\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
