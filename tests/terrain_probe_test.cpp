#include "../core/terrain_resources.hpp"
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
using namespace genesis;
namespace tp=genesis::terrain_probe;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto a=std::make_unique<tp::State>();auto b=std::make_unique<tp::State>();
        std::set<std::uint64_t> fingerprints;
        for(std::uint64_t seed=0;seed<16;++seed) {
            tp::generate(*a,seed);tp::generate(*b,seed);
            std::array<unsigned,5> counts{};
            for(std::size_t i=0;i<a->cells.size();++i) {
                const auto& x=a->cells[i];const auto& y=b->cells[i];
                check(x.elevation==y.elevation&&x.moisture==y.moisture&&x.biome==y.biome,"same seed replay");
                check(x.biome==tp::classify(x.elevation,x.moisture),"classification");
                ++counts[static_cast<unsigned>(x.biome)];
            }
            unsigned kinds=0;for(auto n:counts) kinds+=n!=0;
            check(kinds>=2,"meaningful biome variation");
            fingerprints.insert(tp::fingerprint(*a));
            std::cout<<"TERRAIN_SEED seed="<<seed<<" hash="<<tp::fingerprint(*a)<<" biomes="<<kinds<<'\n';
        }
        check(fingerprints.size()==16,"distinct terrain seeds");
        tp::generate(*a,42);tp::generate(*b,42+(1ULL<<32));
        check(tp::fingerprint(*a)!=tp::fingerprint(*b),"high seed bits influence terrain");
        std::cout<<"TERRAIN_PROBE distinct="<<fingerprints.size()<<" bytes="<<sizeof(*a)
                 <<" seed42="<<tp::fingerprint(*a)<<'\n';
        namespace tr=genesis::terrain_resources;
        const Tile base{100,100,100,100};
        const auto forest=tr::stock_for(base,tp::Biome::forest);
        const auto dry=tr::stock_for(base,tp::Biome::dryland);
        const auto mountain=tr::stock_for(base,tp::Biome::highland);
        check(forest.wood>dry.wood&&forest.food>dry.food&&forest.water>dry.water&&
              mountain.stone>forest.stone,"differentiated natural stocks");
        auto m=std::make_unique<tr::State>();auto n=std::make_unique<tr::State>();
        auto w=std::make_unique<World>();initialize(*w,42);
        const auto rng=w->streams[terrain].state;
        tr::generate(*m,*a,42);tr::generate(*n,*a,42);
        check(tr::valid(*m)&&tr::valid(*n),"valid bounded finite deposits");
        std::array<unsigned,static_cast<unsigned>(tr::Mineral::count)> deposits{};
        for(std::size_t i=0;i<m->deposits.size();++i) {
            const auto& d=m->deposits[i];const auto& e=n->deposits[i];
            check(d.kind==e.kind&&d.remaining==e.remaining&&!d.discovered&&!e.discovered,"same seed hidden deposits");
            ++deposits[static_cast<unsigned>(d.kind)];
        }
        check(deposits[0]>0&&deposits[0]<m->deposits.size(),"uneven finite mineral distribution");
        tr::generate(*n,*a,42+(1ULL<<32));
        bool distinct=false;
        for(std::size_t i=0;i<m->deposits.size();++i)
            distinct|=m->deposits[i].kind!=n->deposits[i].kind||m->deposits[i].remaining!=n->deposits[i].remaining;
        check(distinct,"mineral seed changes deposits");
        const auto expected=tr::stock_for(w->tiles[0],a->cells[0].biome);
        tr::initialize_stocks(*w,*a);
        check(w->tiles[0].food==expected.food&&w->tiles[0].wood==expected.wood&&
              w->tiles[0].water==expected.water&&w->tiles[0].stone==expected.stone&&
              w->streams[terrain].state==rng,"explicit terrain stock initialization preserves RNG");
        std::cout<<"RESOURCE_GEOGRAPHY bytes="<<sizeof(*m)<<" deposits=";
        for(auto count:deposits) std::cout<<count<<',';
        std::cout<<'\n';
        return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
