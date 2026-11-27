#include "../benchmarks/elecrow/fixture.hpp"
#include <iostream>
#include <stdexcept>
using namespace genesis;
int main() {
    try {
        World w{};
        if(benchmark::prepare(w,1,false)||benchmark::prepare(w,257,false))
            throw std::runtime_error("bad population accepted");
        for(auto n:{2u,32u,64u,128u,256u}) {
            for(bool contact:{false,true}) {
                if(!benchmark::prepare(w,n,contact)||w.descendant_count+2!=n)
                    throw std::runtime_error("fixture rejected");
                Production p{};
                for(unsigned i=0;i<600;++i)
                    if(!step_world(w,p)) throw std::runtime_error("tick rejected");
                if(!valid_population(w)) throw std::runtime_error("final state invalid");
            }
        }
        std::cout<<"PASS: disposable benchmark fixtures and 600-tick cases\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;
    }
}
