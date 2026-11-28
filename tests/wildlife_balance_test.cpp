#include "../core/ecology.hpp"
#include "../core/wildlife.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
namespace wild=genesis::wildlife;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
static void observe(World& w,wild::Wildlife& animals,std::uint64_t seed,wild::Cadence cadence,const char* label) {
    initialize(w,seed);wild::initialize(animals,seed);wild::Totals totals{};
    const auto start=wild::population(animals);Production p{};
    for(unsigned i=0;i<ticks_per_year;++i){
        check(step_world(w,p),"authoritative human and renewal tick");
        wild::advance(w,animals,totals,cadence);
        if(w.tick%(ticks_per_year/4)==0) {
            const auto remaining=wild::population(animals);
            check(start+totals.births==remaining+totals.deaths&&wild::valid(animals),"season conservation");
            check(totals.deaths>=totals.predation+totals.hunted+totals.displaced_deaths,"death categories");
            std::array<std::uint64_t,wild::species_count> species{};
            for(const auto& t:animals.tiles) for(unsigned k=0;k<wild::species_count;++k) species[k]+=t.count[k];
            std::cout<<"WILDLIFE_BALANCE seed="<<seed<<" policy="<<label<<" quarter="<<w.tick/(ticks_per_year/4)
                     <<" initial="<<start<<" remaining="<<remaining<<" births="<<totals.births
                     <<" shortage="<<totals.deaths-totals.predation-totals.hunted-totals.displaced_deaths
                     <<" predation="<<totals.predation<<" hunted="<<totals.hunted
                     <<" displaced="<<totals.displaced_deaths<<" food="<<totals.food<<" water="<<totals.water;
            for(auto n:species) std::cout<<" "<<n;
            std::cout<<'\n';
        }
    }
}
int main(){
    try {
        auto w=std::make_unique<World>();auto animals=std::make_unique<wild::Wildlife>();
        check(wild::valid_cadence(wild::sustainable_candidate)&&!wild::valid_cadence({0,600}),"cadence guards");
        initialize(*w,42);const auto i=std::size_t(30)*width+50;
        for(auto& t:w->tiles)t={};
        auto& tile=animals->tiles[i];tile.count[wild::cattle]=2;tile.meat=2;
        auto& stock=w->tiles[i];stock.food=5;stock.water=5;
        // Developed neighbors block migration, isolating the feeding transaction.
        animals->developed.fill(1);animals->developed[i]=0;
        wild::Totals totals{};
        w->tick=60;wild::advance(*w,*animals,totals,wild::sustainable_candidate);
        check(stock.food==5&&tile.count[wild::cattle]==2&&tile.meat==1,"independent spoilage cadence");
        w->tick=600;wild::advance(*w,*animals,totals,wild::sustainable_candidate);
        check(stock.food==3&&stock.water==3&&tile.count[wild::cattle]==2,"actual consumption");
        w->tick=3600;wild::advance(*w,*animals,totals,wild::sustainable_candidate);
        check(stock.food==0&&stock.water==0&&tile.count[wild::cattle]==3&&totals.births==1,"resource-funded breeding");
        w->tick=4200;wild::advance(*w,*animals,totals,wild::sustainable_candidate);
        check(!tile.count[wild::cattle]&&totals.deaths==3,"shortage can still cause extinction");
        stock.food=100;stock.water=100;w->tick=7200;
        wild::advance(*w,*animals,totals,wild::sustainable_candidate);
        check(!tile.count[wild::cattle],"no rescue spawning");
        // Compare the same starting seeds under both cadences, using actual
        // human survival and seasonal renewal, not synthetic resource refills.
        // Outcomes are observational; no target population or seed selection.
        for(const auto seed:{0ULL,42ULL}) {
            observe(*w,*animals,seed,wild::original_cadence,"original");
            observe(*w,*animals,seed,wild::sustainable_candidate,"candidate");
        }
        return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
