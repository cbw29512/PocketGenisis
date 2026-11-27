#include "../core/rng.hpp"
#include <iostream>
#include <stdexcept>
using genesis::Rng;
static bool reference(Rng& r,std::uint32_t limit,std::uint32_t& out) {
    if(!limit) return false;
    const auto n=std::uint64_t{limit};
    const auto threshold=(std::uint64_t{0}-n)%n;
    std::uint64_t value;
    do {value=r.next();} while(value<threshold);
    out=static_cast<std::uint32_t>(value%n);
    return true;
}
int main() {
    try {
        const std::uint32_t bounds[]={0,1,2,3,4,7,21,10000,0x80000001u,0xffffffffu};
        // The first SplitMix output is zero for this initial state, exercising
        // both rejection and accepted low values depending on the bound.
        for(auto seed:{std::uint64_t{0},std::uint64_t{0}-UINT64_C(0x9e3779b97f4a7c15)}) {
            for(auto bound:bounds) {
                Rng a{seed},b{seed};
                for(unsigned i=0;i<1000;++i) {
                    std::uint32_t x=123,y=123;
                    const bool ok_a=a.bounded(bound,x),ok_b=reference(b,bound,y);
                    if(ok_a!=ok_b||x!=y||a.state!=b.state)
                        throw std::runtime_error("bounded RNG stream parity failed");
                }
            }
        }
        std::cout<<"PASS: exact bounded RNG parity including rejection\n";return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
