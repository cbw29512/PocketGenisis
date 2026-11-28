#include "../storage/autosave_clock.hpp"
#include <iostream>
#include <stdexcept>
namespace a=genesis::storage::autosave;
static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(){
    try {
        a::Clock c;
        check(c.poll(90000,100)==a::Status::disarmed,"no implicit boot save");
        check(c.arm(1000,10)&&!c.arm(2000,20),"arm once after recovery");
        check(c.poll(60999,69)==a::Status::waiting,"not early");
        check(c.poll(61000,10)==a::Status::waiting,"unchanged world not rewritten");
        check(c.poll(61000,70)==a::Status::due,"real minute elapsed");
        check(c.complete(61500,70,true)&&c.durable_tick()==70,"verified durable watermark");
        check(c.poll(121499,130)==a::Status::waiting,"interval after completion");
        check(c.poll(121500,130)==a::Status::due,"next save");
        check(!c.complete(122000,130,false)&&c.durable_tick()==70,"failure preserves watermark");
        check(c.poll(999999,131)==a::Status::fault&&!c.arm(999999,131),"fault latched");
        a::Clock rollback;check(rollback.arm(10,50),"arm rollback");
        check(rollback.poll(20,49)==a::Status::fault,"tick rollback");
        a::Clock time;check(time.arm(100,0),"arm clock");
        check(time.poll(99,1)==a::Status::fault,"clock rollback");
        std::cout<<"PASS autosave clock and durability watermark\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
