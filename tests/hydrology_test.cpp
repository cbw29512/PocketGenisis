#include "../core/integrated_world.hpp"
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
            auto sim=std::make_unique<integrated::Simulation>();
            check(sim->create(seed),"balanced initial connected catchments");
            const auto& s=sim->state();
            check(s.founding.report.wet_sinks>=2&&integrated::valid(s),"admitted state valid");
            hy::generate(*twin,s.terrain);
            unsigned channels=0,lakes=0;
            for(std::size_t i=0;i<hy::tile_count;++i) {
                const auto& a=s.drainage.cells[i];const auto& b=twin->cells[i];
                check(a.downstream==b.downstream&&a.area==b.area&&a.runoff==b.runoff,"same terrain drainage");
                channels+=hy::wet(a)&&!hy::lake(a);lakes+=hy::lake(a);
            }
            for(unsigned n=0;n<43200;++n)
                check(sim->step(discovery::probe_cadence),"connected integrated tick");
            check(integrated::valid(s),"integrated conservation");
            std::cout<<"HYDROLOGY_ECOSYSTEM seed="<<seed<<" accepted="<<s.founding.accepted_seed
                     <<" channels="<<channels<<" lakes="<<lakes<<" fish_initial="<<s.fish.initial
                     <<" fish_final="<<fw::population(s.fish)<<" migrated="<<s.fish.migrated
                     <<" caught="<<s.fish.caught<<" crops="<<multi_field::harvests(s.farms)
                     <<" managed="<<livestock::population(s.animals)<<" bytes="<<sizeof(s.drainage)<<'\n';
        }
        std::cout<<"PASS drainage, connected fish recolonization and balanced integrated worlds\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
