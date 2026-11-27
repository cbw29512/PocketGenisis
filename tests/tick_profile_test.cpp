#include "../benchmarks/elecrow/fixture.hpp"
#include "../storage/checkpoint.hpp"
#include <iostream>
#include <stdexcept>
using namespace genesis;
static void check(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
int main() {
    try {
        World normal{},observed{};
        for(bool contact:{false,true}) {
            check(benchmark::prepare(normal,128,contact),"fixture");
            observed=normal;
            for(unsigned i=0;i<600;++i) {
                Production a{},b{};
                std::size_t next=0;
                bool ordered=true;
                check(step_world(normal,a),"normal");
                check(detail::step_world_observed(observed,b,valid_population,[&](TickPhase phase) noexcept {
                    ordered&=static_cast<std::size_t>(phase)==next;
                    ++next;
                }),"observed");
                check(ordered&&next==static_cast<std::size_t>(TickPhase::count),"phase order");
                check(a.food==b.food&&a.water==b.water&&a.wood==b.wood,"production parity");
            }
            check(storage::encode(normal,1)==storage::encode(observed,1),"canonical state and RNG parity");
        }
        std::size_t calls=0;
        Production p{1,2,3};
        observed.tick=std::numeric_limits<std::uint64_t>::max();
        check(!detail::step_world_observed(observed,p,valid_population,[&](TickPhase) noexcept {++calls;}),
              "invalid tick rejected");
        check(calls==0&&!p.food&&!p.water&&!p.wood,"invalid step not observed");
        std::cout<<"PASS: observer phase order, production and canonical replay parity\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;
    }
}
