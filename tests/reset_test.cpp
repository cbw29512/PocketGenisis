#include "ecology.hpp"
#include "../maintenance/host_reset.hpp"
#include <iostream>
#include <fstream>
using namespace genesis;
using namespace genesis::maintenance;
using namespace genesis::storage;
static void check(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
struct Directory {
    std::string path="/tmp/genesis-reset-XXXXXX";
    Directory(){if(!::mkdtemp(path.data())) throw io_error("create test directory");}
    ~Directory(){std::error_code ec;std::filesystem::remove_all(path,ec);}
};
int main() {
    try {
        Directory active,backup,target,other;
        auto w=std::make_unique<World>();initialize(*w,42);Production p{};
        for(int i=0;i<20000;++i)check(step_world(*w,p),"initial tick");
        check(save_host(active.path,*w,true).generation==1,"active checkpoint");
        const Access usb{Transport::physical_usb,true,true};
        for(const auto bad:{Access{Transport::touchscreen,true,true},Access{Transport::network,true,true},
                            Access{Transport::physical_usb,false,true},Access{Transport::physical_usb,true,false}})
            check(!prepare_reset(active.path,*w,bad).error.empty(),"untrusted access refused");
        auto permit=prepare_reset(active.path,*w,usb);
        check(permit.error.empty()&&!permit.snapshot.empty(),"prepare frozen world");
        const auto snapshot=permit.snapshot;
        const auto file=slot_path(backup.path,0);
        check(!stage_reset(permit,usb,*w,active.path,file,target.path,permit.confirmation,43).staged,"missing backup refused");
        publish_file(backup.path,0,snapshot);
        check(!stage_reset(permit,usb,*w,active.path,file,target.path,permit.confirmation,43).staged,"consumed permit refused");
        permit=prepare_reset(active.path,*w,usb);
        check(!stage_reset(permit,usb,*w,active.path,file,target.path,"DESTROY",43).staged,"wrong confirmation refused");
        permit=prepare_reset(active.path,*w,usb);
        check(!stage_reset(permit,{Transport::network,true,true},*w,active.path,file,target.path,permit.confirmation,43).staged,"transport rechecked");
        permit=prepare_reset(active.path,*w,usb);
        check(!stage_reset(permit,usb,*w,active.path,file,target.path,permit.confirmation,42).staged,"same seed refused");
        permit=prepare_reset(active.path,*w,usb);
        check(!stage_reset(permit,usb,*w,active.path,slot_path(active.path,0),target.path,permit.confirmation,43).staged,"source cannot be backup");
        permit=prepare_reset(active.path,*w,usb);
        auto changed=std::make_unique<World>(*w);check(step_world(*changed,p),"changed tick");
        check(!stage_reset(permit,usb,*changed,active.path,file,target.path,permit.confirmation,43).staged,"stale live state refused");
        check(!prepare_reset(active.path,*changed,usb).error.empty(),"unsaved live state refused");
        {std::ofstream out(file,std::ios::trunc);out<<"bad";}
        permit=prepare_reset(active.path,*w,usb);
        check(!stage_reset(permit,usb,*w,active.path,file,target.path,permit.confirmation,43).staged,"corrupt backup refused");
        publish_file(backup.path,0,snapshot);
        permit=prepare_reset(active.path,*w,usb);
        {std::ofstream out(other.path+"/existing");out<<"preserve";}
        check(!stage_reset(permit,usb,*w,active.path,file,other.path,permit.confirmation,43).staged,"nonempty target refused");
        permit=prepare_reset(active.path,*w,usb);
        check(stage_reset(permit,usb,*w,active.path,file,target.path,permit.confirmation,43).staged,"authorized replacement staged");
        auto fresh=load_host(target.path);
        check(fresh.state.world&&!fresh.degraded&&fresh.state.world->seed==43&&fresh.state.world->tick==0&&
              fresh.state.world->descendant_count==0&&fresh.state.world->archive_count==0&&fresh.state.world->founders[0].id==1&&
              fresh.state.world->founders[1].id==2,"new founders and clean world");
        check(read_file(slot_path(active.path,0)).bytes==snapshot&&read_file(file).bytes==snapshot,"active world and backup untouched");
        check(!stage_reset(permit,usb,*w,active.path,file,target.path,permit.confirmation,44).staged,"success cannot replay");
        std::cout<<"PASS: reset authorization, stale state, exact backup, isolated replacement and replay protection\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
