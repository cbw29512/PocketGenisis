#include "../storage/identified_checkpoint.hpp"
#include <iostream>
using namespace genesis;
namespace c=genesis::storage::identified_checkpoint;
namespace w=genesis::storage::world_identity;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        w::Uuid entropy{};entropy[0]=1;
        const auto id=w::make(entropy,123,true);
        entropy[0]=2;const auto other=w::make(entropy,123,true);
        auto sim=std::make_unique<integrated::Simulation>();
        check(sim->create(42),"create");
        const auto a=c::encode(id,*sim,1);
        for(unsigned i=0;i<100;++i) check(sim->step(60),"step");
        const auto b=c::encode(id,*sim,2);
        {auto r=c::recover(a,b,id);
         check(r.checkpoint.slot==1&&r.checkpoint.state.generation==2&&w::equal(r.identity,id),"identity roundtrip");
         check(c::encode(r.identity,*r.checkpoint.state.simulation,2)==b,"exact envelope roundtrip");}
        {auto r=c::recover(a,c::encode(other,*sim,2),id);
         check(!r.checkpoint.state.simulation,"same seed foreign UUID refused");}
        check(!c::recover(a,b,other).checkpoint.state.simulation,"wrong expected UUID refused");
        auto torn=b;torn.pop_back();
        {auto r=c::recover(a,torn,id);check(r.checkpoint.slot==0&&r.checkpoint.degraded,"torn envelope fallback");}
        auto future=b;future[4]=99;
        const auto crc=storage::crc32(future,future.size()-4);
        for(unsigned i=0;i<4;++i) future[future.size()-4+i]=static_cast<std::uint8_t>(crc>>(8*i));
        {auto r=c::recover(a,future,id);check(r.checkpoint.migration_required&&!r.checkpoint.state.simulation,"future envelope protected");}
        check(!c::recover({}, {},id).checkpoint.state.simulation,"missing never creates");
        std::cout<<"PASS identified checkpoint binding and recovery fixtures\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
