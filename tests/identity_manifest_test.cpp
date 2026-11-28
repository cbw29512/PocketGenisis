#include "../storage/identity_manifest.hpp"
#include <iostream>
using namespace genesis::storage;
namespace m=identity_manifest;
namespace w=world_identity;
static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static void repair_crc(Bytes& bytes){
    const auto crc=crc32(bytes,bytes.size()-4);
    for(unsigned i=0;i<4;++i)bytes[bytes.size()-4+i]=static_cast<std::uint8_t>(crc>>(8*i));
}
int main(){
    try{
        w::Uuid entropy{};entropy[0]=1;const auto id=w::make(entropy,123,true);
        const auto bytes=m::encode(id);const auto decoded=m::decode(bytes);
        check(bytes.size()==m::size&&decoded.valid&&w::equal(decoded.identity,id),"identity roundtrip");
        check(m::encode(decoded.identity)==bytes,"canonical bytes");
        for(std::size_t i=0;i<bytes.size();++i){auto bad=bytes;bad[i]^=1;check(!m::decode(bad).valid,"every byte corruption rejected");}
        for(std::size_t n=0;n<bytes.size();++n){auto bad=bytes;bad.resize(n);check(!m::decode(bad).valid,"all truncations rejected");}
        auto bad=bytes;bad[4]=99;repair_crc(bad);check(!m::decode(bad).valid,"unknown version rejected");
        bad=bytes;bad[30]=2;repair_crc(bad);check(!m::decode(bad).valid,"invalid flag rejected");
        bad=bytes;bad[12]=0;repair_crc(bad);check(!m::decode(bad).valid,"invalid UUID rejected");
        std::cout<<"PASS identity manifest roundtrip and corruption guards\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
