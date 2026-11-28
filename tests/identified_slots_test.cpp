#include "../storage/identified_slots.hpp"
#include <iostream>
using namespace genesis;
namespace c=genesis::storage::identified_checkpoint;
namespace w=genesis::storage::world_identity;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
struct MemoryAdapter {
    c::SlotRead slots[2]{{{},true},{{},true}};
    bool fail_read{},tear{};
    c::SlotRead read(int n) {
        if(fail_read) throw std::runtime_error("storage unavailable");
        return slots[n];
    }
    void publish(int n,const storage::Bytes& bytes) {
        slots[n]={bytes,false};
        if(tear) {slots[n].bytes.pop_back();throw std::runtime_error("interrupted write");}
    }
};
int main(){
    try {
        MemoryAdapter io;w::Uuid entropy{};entropy[0]=1;
        const auto id=w::make(entropy,123,true);
        auto sim=std::make_unique<integrated::Simulation>();
        check(sim->create(42),"create");
        check(!c::save(io,id,*sim).generation,"explicit creation required");
        check(c::save(io,id,*sim,true).generation==1,"first save");
        for(unsigned i=0;i<100;++i) check(sim->step(60),"step");
        io.tear=true;
        check(!c::save(io,id,*sim).generation,"interrupted write reported");
        {auto r=c::load(io,id);check(r.checkpoint.slot==0&&r.checkpoint.degraded&&r.checkpoint.state.generation==1,"old slot survives");}
        io.tear=false;
        check(c::save(io,id,*sim).generation==2,"repair inactive slot");
        {auto r=c::load(io,id);check(r.checkpoint.slot==1&&r.checkpoint.state.generation==2,"latest selected");}
        io.fail_read=true;
        check(!c::save(io,id,*sim,true).generation,"missing media fails closed");
        io.fail_read=false;
        entropy[0]=2;
        check(!c::save(io,w::make(entropy,123,true),*sim,true).generation,"foreign identity refused");
        MemoryAdapter empty;
        check(!c::save(empty,id,*sim,true).generation,"advanced world cannot implicitly initialize");
        std::cout<<"PASS alternating slot transactions and interrupted-write recovery\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
