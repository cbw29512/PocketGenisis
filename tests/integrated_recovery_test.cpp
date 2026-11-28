#include "../storage/integrated_recovery.hpp"
#include <iostream>
using namespace genesis;
namespace c=genesis::storage::integrated_checkpoint;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto sim=std::make_unique<integrated::Simulation>();
        check(sim->create(42),"create");
        auto a=c::encode(*sim,1);
        for(unsigned i=0;i<100;++i) check(sim->step(60),"advance");
        auto b=c::encode(*sim,2);
        {auto r=c::recover(a,b);check(r.slot==1&&!r.degraded&&r.state.generation==2,"latest valid");}
        auto torn=b;torn.pop_back();
        {auto r=c::recover(a,torn);check(r.slot==0&&r.degraded&&r.state.generation==1,"torn latest fallback");}
        check(!c::recover({},{}).state.simulation,"empty never founds");
        check(!c::recover(a,c::encode(*sim,1)).state.simulation,"equal generation conflict");
        auto original=c::decode(a);
        check(original.simulation!=nullptr,"decode original");
        check(!c::recover(b,c::encode(*original.simulation,3)).state.simulation,"time reversal refused");
        auto foreign=std::make_unique<integrated::Simulation>();
        check(foreign->create(43),"foreign creation");
        check(!c::recover(a,c::encode(*foreign,2)).state.simulation,"foreign founding refused");
        auto future=b;future[4]=99;
        auto crc=storage::crc32(future,future.size()-4);
        for(unsigned i=0;i<4;++i) future[future.size()-4+i]=static_cast<std::uint8_t>(crc>>(8*i));
        auto r=c::recover(a,future);
        check(!r.state.simulation&&r.migration_required,"future format protected");
        std::cout<<"PASS integrated recovery selection and fail-closed conflicts\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
