#include "../maintenance/reset_transaction.hpp"
#include <iostream>
namespace m=genesis::maintenance;
struct Fake {
    std::string id="old-world",marker;
    bool world=true,unrelated=true,interrupt=false;
    std::string identity()const{return id;}
    bool pending()const{return !marker.empty();}
    std::string reset_identity()const{return marker;}
    void begin_reset(const std::string& value){marker=value;}
    void erase_world_files(){if(interrupt)throw std::runtime_error("power loss");world=false;}
    bool world_absent()const{return !world;}
    void finish_reset(){marker.clear();}
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
        auto valid=m::prepare_reset(io.id,true);io.interrupt=true;
        check(m::execute_reset(io,valid,valid.confirmation,true).status==m::ResetStatus::incomplete&&io.pending(),"interrupted reset marker");
        check(m::resume_reset(io,"bad",true).status==m::ResetStatus::refused&&io.world,"resume requires confirmation");
        io.interrupt=false;
        check(m::resume_reset(io,valid.confirmation,true).status==m::ResetStatus::empty&&!io.pending()&&!io.world&&io.unrelated,"resume preserves unrelated data");
        Fake fresh;auto p=m::prepare_reset(fresh.id,true);
        check(m::execute_reset(fresh,p,p.confirmation,true).status==m::ResetStatus::empty&&fresh.unrelated,"normal reset leaves empty world");
        std::cout<<"PASS reset confirmation and interrupted transaction fixtures\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
