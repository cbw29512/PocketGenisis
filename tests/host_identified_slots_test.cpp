#include "../storage/host_identified_slots.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace genesis;
namespace c=genesis::storage::identified_checkpoint;
namespace w=genesis::storage::world_identity;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
struct Directory {
    std::string path="/tmp/genesis-identified-XXXXXX";
    Directory(){if(!::mkdtemp(path.data())) throw std::runtime_error("temporary directory failed");}
    ~Directory(){std::error_code ec;std::filesystem::remove_all(path,ec);}
};
int main(){
    try {
        Directory d;c::HostSlots io(d.path);
        w::Uuid entropy{};entropy[0]=1;const auto id=w::make(entropy,123,true);
        auto sim=std::make_unique<integrated::Simulation>();
        check(sim->create(42),"create");
        check(c::save(io,id,*sim,true).generation==1,"initial durable write");
        for(unsigned i=0;i<100;++i) check(sim->step(60),"advance");
        check(c::save(io,id,*sim).generation==2,"alternate durable write");
        {c::HostSlots reopened(d.path);auto r=c::load(reopened,id);
         check(r.checkpoint.state.simulation&&r.checkpoint.state.generation==2,"reopen latest");}
        {std::ofstream out(d.path+"/world-b.pgid",std::ios::trunc);out<<"torn";}
        {auto r=c::load(io,id);check(r.checkpoint.slot==0&&r.checkpoint.degraded,"recover prior");}
        check(c::save(io,id,*sim).generation==2,"repair");
        c::HostSlots missing(d.path+"/absent");
        check(!c::save(missing,id,*sim,true).generation,"missing directory not empty media");
        std::cout<<"PASS POSIX identified checkpoint persistence and recovery\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
