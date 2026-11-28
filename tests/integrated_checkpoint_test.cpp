#include "../storage/integrated_checkpoint.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
namespace codec=genesis::storage::integrated_checkpoint;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto a=std::make_unique<integrated::Simulation>();
        check(a->create(42),"explicit integrated creation");
        const auto genesis=codec::encode(*a,1);
        auto zero=codec::decode(genesis);
        check(zero.simulation&&codec::encode(*zero.simulation,1)==genesis,"tick-zero exact roundtrip");
        check(!zero.simulation->create(9),"restored world cannot be reset");
        for(unsigned n=0;n<21600;++n) check(a->step(60),"pre-checkpoint tick");
        const auto bytes=codec::encode(*a,2);
        auto restored=codec::decode(bytes);
        check(restored.simulation&&restored.generation==2&&codec::encode(*restored.simulation,2)==bytes,
              "all encoded fields roundtrip exactly");
        check(!restored.simulation->step(61),"restored cadence is immutable");
        for(unsigned n=0;n<21600;++n) {
            check(a->step(60)&&restored.simulation->step(60),"continuous and restored ticks");
            if(n%3600==0) check(codec::encode(*a,3)==codec::encode(*restored.simulation,3),"canonical replay");
        }
        check(codec::encode(*a,3)==codec::encode(*restored.simulation,3),"full annual checkpoint replay");
        auto damaged=bytes;damaged[100]^=1;
        check(!codec::decode(damaged).simulation,"CRC corruption rejected");
        damaged=bytes;damaged.pop_back();
        check(!codec::decode(damaged).simulation,"truncation rejected");
        damaged=bytes;damaged[4]=99;
        const auto crc=storage::crc32(damaged,damaged.size()-4);
        for(unsigned i=0;i<4;++i) damaged[damaged.size()-4+i]=static_cast<std::uint8_t>(crc>>(8*i));
        check(!codec::decode(damaged).simulation,"unknown valid-CRC version rejected");
        std::cout<<"INTEGRATED_CHECKPOINT bytes="<<bytes.size()<<" replay_ticks="<<a->state().world.tick
                 <<" generation="<<restored.generation<<'\n';
        std::cout<<"PASS versioned integrated checkpoint roundtrip, replay and fail-closed rejection\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
