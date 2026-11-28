// Isolated disposable capacity; never link with default World definitions.
#define PG_DISPOSABLE_SCALE_PROBE
#define PG_DISPOSABLE_4096_PROBE
#include "../benchmarks/elecrow/fixture.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
int main() {
    try {
        static_assert(live_capacity==4098 && history_capacity==8192);
        auto w=std::unique_ptr<World>(new World);
        if(benchmark::prepare(*w,4099,false))
            throw std::runtime_error("invalid capacity admitted");
        for(bool contact:{false,true}) {
            if(!benchmark::prepare(*w,4096,contact)||w->descendant_count+2!=4096)
                throw std::runtime_error("4096 fixture rejected");
            // Hit the disease pulse and conception opportunity in a bounded test.
            w->tick+=599;
            Production p{};
            if(!step_world(*w,p)||!valid_population(*w))
                throw std::runtime_error("4096 boundary tick rejected");
        }
        std::cout<<"PASS: 4096 ordinary and crowded boundary ticks\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;
    }
}
