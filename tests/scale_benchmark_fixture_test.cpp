// This single translation unit intentionally uses a disposable larger World.
// Never combine it with default-capacity object files or persistence code.
#define PG_DISPOSABLE_SCALE_PROBE
#include "../benchmarks/elecrow/fixture.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
int main() {
    try {
        static_assert(live_capacity==2050 && history_capacity==4096);
        auto w=std::make_unique<World>();
        if(benchmark::prepare(*w,2051,false))
            throw std::runtime_error("invalid capacity admitted");
        for(auto n:{256u,512u,1024u,2048u}) {
            for(bool contact:{false,true}) {
                if(!benchmark::prepare(*w,n,contact)||w->descendant_count+2!=n)
                    throw std::runtime_error("fixture rejected");
                // Cross disease pulses and a conception opportunity.
                w->tick+=480;
                Production p{};
                for(unsigned i=0;i<120;++i)
                    if(!step_world(*w,p)) throw std::runtime_error("tick rejected");
                if(!valid_population(*w)) throw std::runtime_error("invalid final state");
            }
        }
        std::cout<<"PASS: scale fixtures, disease pulses and conception boundary\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;
    }
}
