#include "../storage/world_identity.hpp"
#include <iostream>
namespace w=genesis::storage::world_identity;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        w::Uuid a{};a[0]=1;
        auto b=a;b[0]=2;
        const auto one=w::make(a,123,true),two=w::make(b,123,true);
        check(w::valid(one)&&w::valid(two)&&!w::equal(one,two),"independent identities");
        check(w::equal(one,w::make(a,123,true)),"stable supplied metadata");
        check(!w::equal(one,w::make(a,124,true)),"creation date bound");
        check(!w::equal(one,w::make(a,123,false)),"official status bound");
        check(w::valid(w::make(a,0,false)),"disposable clock optional");
        check(!w::valid(w::Identity{}),"blank identity refused");
        bool rejected=false;
        try{(void)w::make({},123,true);}catch(const std::runtime_error&){rejected=true;}
        check(rejected,"missing entropy refused");
        rejected=false;
        try{(void)w::make(a,0,true);}catch(const std::runtime_error&){rejected=true;}
        check(rejected,"official creation time required");
        std::cout<<"PASS independent world identity metadata\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
