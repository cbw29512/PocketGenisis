#include "../storage/host_archive_cycle.hpp"
#include <filesystem>
#include <iostream>
using namespace genesis;
using namespace genesis::storage;
static void check(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
template<class F> static void rejects(F f,const char* why) {
    try {f();} catch(const std::exception&) {return;}
    throw std::runtime_error(why);
}
struct Directory {
    std::string path="/tmp/genesis-budget-XXXXXX";
    Directory(){if(!::mkdtemp(path.data())) throw io_error("create test directory");}
    ~Directory(){std::error_code ec;std::filesystem::remove_all(path,ec);}
};
int main() {
    try {
        Directory dir,empty,full;
        const auto fs=storage_space(dir.path);
        check(fs.block>0&&fs.available>0,"real filesystem measurement");
        check(round_storage(0,4096)==0&&round_storage(4097,4096)==8192,"block rounding");
        check(transaction_storage_bytes(10003,64,4096)>transaction_storage_bytes(3,0,4096),"growth accounted");
        rejects([&]{transaction_storage_bytes(UINT64_MAX,0,4096);},"future ID overflow");
        rejects([&]{transaction_storage_bytes(3,history_capacity+1,4096);},"invalid retirement count");
        rejects([&]{round_storage(1,0);},"invalid block size");
        auto w=std::make_unique<World>();initialize(*w,42);
        rejects([&]{create_archive_host(empty.path,*w,StorageBudget{0});},"creation refused under budget");
        check(std::filesystem::is_empty(empty.path),"no implicit checkpoint on refusal");
        auto original=create_archive_host(dir.path,*w);
        rejects([&]{run_archive_cycle(dir.path,1,StorageBudget{0});},"low-space cycle refused");
        check(load_archive_host(dir.path).committed==original.committed,"no progress lost on refusal");
        const auto required=transaction_storage_bytes(w->next_citizen_id,0,fs.block);
        rejects([&]{advance_archive_host(dir.path,original,1,StorageBudget{required-1});},"one byte below budget");
        check(load_archive_host(dir.path).committed==original.committed,"insufficient budget leaves checkpoint");
        auto resumed=run_archive_cycle(dir.path,1);
        check(resumed.state.world->tick==1,"resume with sufficient budget");
        w->tick=100;w->archive_count=history_capacity;w->next_citizen_id=history_capacity+3;
        for(std::size_t i=0;i<w->archive_count;++i) {
            Citizen c{};c.id=i+3;c.parent1=1;c.parent2=2;c.birth_tick=1;
            c.health=0;c.thirst=100;w->archive[i]={c,2,DeathCause::dehydration};
        }
        auto archived=create_archive_host(full.path,*w);
        rejects([&]{run_archive_cycle(full.path,1,StorageBudget{0});},"retirement refused before batch write");
        check(read_file(history_path(full.path,0)).missing&&
              load_archive_host(full.path).committed==archived.committed,"no partial publication on preflight failure");
        publish_segment(full.path,extract_segment(*w,0,17));
        const auto pending=read_file(history_path(full.path,0)).bytes;
        rejects([&]{run_archive_cycle(full.path,1,StorageBudget{0});},"pending retry cannot bypass budget");
        check(read_file(history_path(full.path,0)).bytes==pending&&
              load_archive_host(full.path).committed==archived.committed,"interrupted transfer preserved");
        auto recovered=run_archive_cycle(full.path,1);
        check(recovered.action==CycleAction::resumed_retirement&&recovered.state.history->count()==17,
              "retry succeeds after storage available");
        std::cout<<"PASS: checked storage arithmetic, capacity refusal before publication, preserved history and resume\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
