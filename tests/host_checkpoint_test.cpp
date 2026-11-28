#include "ecology.hpp"
#include "../storage/host_checkpoint.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace genesis;
using namespace genesis::storage;
static void check(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
struct Directory {
    std::string path="/tmp/genesis-checkpoint-XXXXXX";
    Directory(){if(!::mkdtemp(path.data())) throw io_error("create test directory");}
    ~Directory(){std::error_code ec;std::filesystem::remove_all(path,ec);}
};
int main() {
    try {
        Directory dir;auto w=std::make_unique<World>();initialize(*w,42);Production p{};
        check(!load_host(dir.path).state.world,"empty directory must not initialize");
        check(!save_host(dir.path,*w).generation,"explicit creation required");
        check(save_host(dir.path,*w,true).generation==1,"first durable write");
        for(int i=0;i<20000;++i)check(step_world(*w,p),"simulation tick");
        check(save_host(dir.path,*w).generation==2,"second durable write");
        auto latest=load_host(dir.path);
        check(latest.slot==1&&latest.state.world&&encode(*latest.state.world,2)==encode(*w,2),"latest file roundtrip");
        {std::ofstream out(slot_path(dir.path,1),std::ios::binary|std::ios::trunc);out<<"torn write";}
        auto fallback=load_host(dir.path);
        check(fallback.slot==0&&fallback.degraded&&fallback.state.world&&fallback.state.world->tick==0,"fallback after truncated file");
        check(save_host(dir.path,*w).generation==2,"repair invalid slot, retain older good copy");
        check(load_host(dir.path).state.generation==2,"repaired checkpoint loads");
        check(!save_host(dir.path,*fallback.state.world).generation,"backwards world refused");
        w->founders[0].hunger=static_cast<std::uint8_t>((w->founders[0].hunger+1)%100);
        check(!save_host(dir.path,*w).generation,"different state at same tick refused");
        auto future=encode(*latest.state.world,3);future[4]=2;
        const auto crc=crc32(future,future.size()-4);
        for(unsigned i=0;i<4;++i)future[future.size()-4+i]=static_cast<std::uint8_t>(crc>>(8*i));
        {std::ofstream out(slot_path(dir.path,0),std::ios::binary|std::ios::trunc);
         out.write(reinterpret_cast<const char*>(future.data()),static_cast<std::streamsize>(future.size()));}
        check(load_host(dir.path).state.unsupported&&!save_host(dir.path,*w,true).generation,"unsupported save protected from overwrite");
        initialize(*w,43);check(!save_host(dir.path,*w).generation,"foreign seed refused");
        {std::ofstream out(slot_path(dir.path,0),std::ios::trunc);out<<"bad";}
        {std::ofstream out(slot_path(dir.path,1),std::ios::trunc);out<<"bad";}
        check(!save_host(dir.path,*w,true).generation&&!load_host(dir.path).state.world,"never silently reset corrupt world");
        std::cout<<"PASS: host durable writes, readback, alternate slots, fallback, repair and no silent reset\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
