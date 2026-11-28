#include "../maintenance/reset_transaction.hpp"
#include <iostream>
namespace m=genesis::maintenance;
struct Fake {
    std::string id="old-world",marker;
    bool world=true,unrelated=true,interrupt=false,wrong_marker=false,finish_fault=false;
    unsigned erases{};
    std::string identity()const{return id;}
    bool pending()const{return !marker.empty();}
    std::string reset_identity()const{return marker;}
    void begin_reset(const std::string& value){marker=wrong_marker?"foreign":value;}
    void erase_world_files(const std::string& expected){
        if(id!=expected||marker!=expected)throw std::runtime_error("foreign identity");
        if(interrupt)throw std::runtime_error("power loss");
        ++erases;world=false;
    }
    bool world_absent()const{return !world;}
    void finish_reset(){if(finish_fault)throw std::runtime_error("marker clear failed");marker.clear();}
};
static void check(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
int main(){
    try{
        Fake io;
        auto permit=m::prepare_reset(io.id,true);
        check(m::execute_reset(io,permit,"bad",true).status==m::ResetStatus::refused&&io.world,"wrong confirmation");
        check(m::execute_reset(io,permit,permit.confirmation,true).status==m::ResetStatus::refused,"consumed permit");
        auto changed=m::prepare_reset("foreign",true);
        check(m::execute_reset(io,changed,changed.confirmation,true).status==m::ResetStatus::refused&&io.world,"changed identity");
        auto forged=m::prepare_reset(io.id,true);forged.confirmation="bad";
        check(m::execute_reset(io,forged,"bad",true).status==m::ResetStatus::refused&&io.world,"noncanonical confirmation");
        auto valid=m::prepare_reset(io.id,true);io.interrupt=true;
        check(m::execute_reset(io,valid,valid.confirmation,true).status==m::ResetStatus::incomplete&&io.pending(),"interrupted reset marker");
        check(m::resume_reset(io,"bad",true).status==m::ResetStatus::refused&&io.world,"resume requires confirmation");
        io.interrupt=false;
        check(m::resume_reset(io,valid.confirmation,true).status==m::ResetStatus::empty&&!io.pending()&&!io.world&&io.unrelated,"resume preserves unrelated data");
        Fake fresh;auto p=m::prepare_reset(fresh.id,true);
        check(m::execute_reset(fresh,p,p.confirmation,true).status==m::ResetStatus::empty&&fresh.unrelated,"normal reset leaves empty world");
        Fake bad;bad.wrong_marker=true;auto q=m::prepare_reset(bad.id,true);
        check(m::execute_reset(bad,q,q.confirmation,true).status==m::ResetStatus::incomplete&&bad.world&&bad.erases==0,"wrong marker blocks deletion");
        Fake end;end.finish_fault=true;auto r=m::prepare_reset(end.id,true);
        check(m::execute_reset(end,r,r.confirmation,true).status==m::ResetStatus::incomplete&&!end.world&&end.pending(),"marker clear failure");
        end.finish_fault=false;
        check(m::resume_reset(end,r.confirmation,true).status==m::ResetStatus::empty&&end.erases==1&&!end.pending(),"resume only clears marker after deletion");
        std::cout<<"PASS reset confirmation and interrupted transaction fixtures\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
