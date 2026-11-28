#include "ecology.hpp"
#include "../storage/checkpoint.hpp"
#include <iostream>
#include <stdexcept>
using namespace genesis;
using namespace genesis::storage;
static void check(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
static void repair_crc(Bytes& b) {
    const auto crc=crc32(b,b.size()-4);
    for(unsigned i=0;i<4;++i)b[b.size()-4+i]=static_cast<std::uint8_t>(crc>>(8*i));
}
int main() {
    try {
        auto w=std::make_unique<World>();initialize(*w,42);Production p{};
        for(int i=0;i<50000;++i)check(step_world(*w,p),"initial simulation");
        auto a=encode(*w,1);auto decoded=decode(a);
        check(decoded.world&&decoded.generation==1&&encode(*decoded.world,1)==a,"complete field roundtrip");
        for(int i=0;i<10000;++i)check(step_world(*w,p)&&step_world(*decoded.world,p),"replay steps");
        auto b=encode(*w,2);
        check(b==encode(*decoded.world,2),"canonical continuation after restore");
        auto recovered=recover(a,b);
        check(recovered.slot==1&&!recovered.degraded&&recovered.state.generation==2,"newest checkpoint chosen");
        auto damaged=b;damaged[100]^=1;
        recovered=recover(a,damaged);
        check(recovered.slot==0&&recovered.degraded&&recovered.state.generation==1,"corruption fallback");
        damaged=b;damaged.pop_back();check(!decode(damaged).world,"truncated rejected");
        damaged=b;damaged[4]=2;repair_crc(damaged);check(!decode(damaged).world,"unknown version rejected even with valid CRC");
        check(!recover(a,damaged).state.world&&recover(a,damaged).state.unsupported,"unknown version is not a corruption fallback");
        // Check an explicitly invalid founder coordinate using known field order.
        damaged=b;const auto founder_offset=16+8*4+8*stream_count+8*width*height;
        damaged[founder_offset+8]=255;damaged[founder_offset+9]=255;repair_crc(damaged);
        check(!decode(damaged).world,"invalid semantic state rejected despite CRC");
        check(!recover(Bytes{},Bytes{}).state.world,"no silent creation");
        auto other=std::make_unique<World>();initialize(*other,99);
        check(!recover(a,encode(*other,2)).state.world,"mixed seed rejected");
        check(!recover(a,encode(*w,1)).state.world,"equal generation conflict rejected");
        check(!recover(b,encode(*decode(a).world,3)).state.world,"backwards tick rejected");
        std::cout<<"PASS: checkpoint roundtrip, replay, corruption, versions, semantic validation and generation recovery\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
