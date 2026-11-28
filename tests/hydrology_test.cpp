#define PG_HOUSING_PROBE
#define PG_AGRICULTURE_PROBE
#define PG_IRRIGATION_PROBE
#define PG_MULTI_FIELD_PROBE
#define PG_CROP_LOSS_PROBE
#define PG_LIVESTOCK_PROBE
#define PG_SOIL_FERTILITY_PROBE
#define PG_MANURE_PROBE
#define PG_FATIGUE_PROBE
#define PG_REPRODUCTIVE_DRIVE_PROBE
#include "../core/terrain_renewal.hpp"
#include "../core/world_admission.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
namespace hy=genesis::hydrology;
namespace fw=genesis::freshwater;
namespace tp=genesis::terrain_probe;
namespace tr=genesis::terrain_resources;
namespace build=genesis::construction;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto w=std::make_unique<World>();initialize(*w,42);
        auto terrain=std::make_unique<tp::State>();
        auto network=std::make_unique<hy::State>();
        auto twin=std::make_unique<hy::State>();
        auto fish=std::make_unique<fw::State>();
        auto minerals=std::make_unique<tr::State>();
        auto wild=std::make_unique<wildlife::Wildlife>();
        // Moist descending strip surrounded by dry flat terrain. Two
        // independent wet catchments run side by side, separated by dry row.
        for(auto& c:terrain->cells)c={220,0,tp::Biome::highland};
        constexpr std::size_t start=20*width+20;
        for(unsigned n=0;n<24;++n) for(unsigned offset:{0u,2u*width}) {
            const auto h=static_cast<std::uint8_t>(150-n);
            terrain->cells[start+n+offset]={h,150,tp::classify(h,150)};
        }
        hy::generate(*network,*terrain);
        check(hy::valid(*network,*terrain),"drainage conservation");
        const auto upstream=start+12,downstream=upstream+1;
        check(hy::connected(*network,upstream,downstream)&&
              !hy::connected(*network,upstream,upstream+2*width)&&
              hy::lake(network->cells[start+23]),"connected channel and terminal lake");
        network->cells[upstream].area++;
        check(!hy::valid(*network,*terrain),"corrupt catchment rejected");
        hy::generate(*network,*terrain);
        for(auto& t:w->tiles)t.water=100;
        fw::initialize(*fish,*w,*network);
        check(fw::valid(*fish)&&fish->cells[upstream].fish>=2&&
              !fish->cells[upstream+width].habitat,"fish restricted to wet drainage");
        // Empty every habitat except a source. Preserve the real catch
        // ledger, and bring source to four via ordinary breeding if needed.
        for(std::size_t i=0;i<fish->cells.size();++i) if(i!=upstream)
            while(fw::catch_one(*fish,i)) {}
        for(auto& t:w->tiles)t.water=0;
        w->tiles[upstream].water=100;
        for(unsigned n=1;n<=3&&fish->cells[upstream].fish<4;++n) {
            w->tick=3600*n;fw::pulse(*w,*fish,true,network.get());
        }
        check(fish->cells[upstream].fish>=4&&fw::valid(*fish),"source breeds using real forage");
        // Only one connected target has adequate water; other basin has
        // water but no route. A breeding-size transfer must be conserved.
        w->tiles[downstream].water=100;
        w->tiles[upstream+2*width].water=100;
        w->tick=14400; // even origin parity
        const auto before=fw::population(*fish);
        fw::migrate(*w,*fish,false,network.get());
        check(fish->cells[downstream].fish==2&&fish->cells[upstream].fish>=2&&
              !fish->cells[upstream+2*width].fish&&fw::population(*fish)==before&&
              fw::valid(*fish),"genuine connected recolonization");
        for(std::size_t i=0;i<fish->cells.size();++i) while(fw::catch_one(*fish,i)) {}
        w->tick+=3600;fw::pulse(*w,*fish,true,network.get());
        check(!fw::population(*fish)&&fw::valid(*fish),"global extinction remains permanent");
        for(auto seed:{0ULL,42ULL}) {
            const auto admitted=world_admission::generate_candidate(*w,*terrain,*minerals,*wild,*fish,
                seed,world_admission::max_attempts,network.get());
            check(admitted.accepted&&admitted.report.wet_sinks>=2&&hy::valid(*network,*terrain),
                  "balanced initial connected catchments");
            hy::generate(*twin,*terrain);
            unsigned channels=0,lakes=0;
            for(std::size_t i=0;i<hy::tile_count;++i) {
                const auto& a=network->cells[i];const auto& b=twin->cells[i];
                check(a.downstream==b.downstream&&a.area==b.area&&a.runoff==b.runoff,"same terrain drainage");
                channels+=hy::wet(a)&&!hy::lake(a);lakes+=hy::lake(a);
            }
            discovery::State knowledge{};build::State sites{};storage::State reserve{};
            cooking::State cooked{};shelter::State sheltered{};settlement::State home{};
            multi_field::State farms{};irrigation::State water{};livestock::State animals{};
            wildlife::Totals ecology{};Production p{};discovery::Event re{};build::Event ce{};
            regional_weather::Cache climate(w->seed);
            const terrain_renewal::Policy policy{*terrain,{sites},*wild,&climate};
            for(unsigned n=0;n<43200;++n)
                check(livestock::step_with_renewal(*w,knowledge,sites,reserve,cooked,sheltered,
                      home,farms,water,*wild,ecology,animals,p,re,ce,discovery::probe_cadence,
                      policy,fish.get(),true,network.get()),"connected integrated tick");
            check(fw::valid(*fish)&&valid_population(*w)&&multi_field::valid(farms)&&
                  livestock::valid(animals)&&hy::valid(*network,*terrain),"integrated conservation");
            std::cout<<"HYDROLOGY_ECOSYSTEM seed="<<seed<<" accepted="<<admitted.accepted_seed
                     <<" channels="<<channels<<" lakes="<<lakes<<" fish_initial="<<fish->initial
                     <<" fish_final="<<fw::population(*fish)<<" migrated="<<fish->migrated
                     <<" caught="<<fish->caught<<" crops="<<multi_field::harvests(farms)
                     <<" managed="<<livestock::population(animals)<<" bytes="<<sizeof(*network)<<'\n';
        }
        std::cout<<"PASS drainage, connected fish recolonization and balanced integrated worlds\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
