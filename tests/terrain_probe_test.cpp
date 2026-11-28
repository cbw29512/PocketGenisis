#include "../core/terrain_probe.hpp"
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
        return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
