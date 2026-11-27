#include "../core/construction_work.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
namespace build=genesis::construction;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto w=std::unique_ptr<World>(new World);
        initialize(*w,42);discovery::State knowledge{};build::State sites{};
        w->tick=1;
        check(build::advance(*w,knowledge,sites).kind==build::EventKind::none,"knowledge prerequisite");
        knowledge.known[0]=discovery::tech_bit(discovery::fire)|discovery::tech_bit(discovery::shelter);
        auto& c=w->founders[0];auto& stock=discovery::local(*w,c);
        stock.wood=0;stock.stone=20;
        check(build::advance(*w,knowledge,sites).kind==build::EventKind::none,"material prerequisite");
        stock.wood=20;
        auto e=build::advance(*w,knowledge,sites);
        check(e.kind==build::EventKind::started&&e.type==build::campfire,"admission");
        check(stock.wood==16&&stock.stone==18,"debit once");
        c.hunger=12;++w->tick;build::advance(*w,knowledge,sites);
        check(sites.sites[0].work==0,"survival priority");
        c.hunger=0;
        for(unsigned i=0;i<20;++i){++w->tick;e=build::advance(*w,knowledge,sites);}
        check(e.kind==build::EventKind::completed&&build::completed(sites)==1,"work completion");
        ++w->tick;
        check(build::advance(*w,knowledge,sites).kind==build::EventKind::none,"occupied tile");
        ++c.x;++w->tick;
        check(build::advance(*w,knowledge,sites).kind==build::EventKind::started,"new tile");
        c.hunger=100;w->tick+=600;
        check(build::advance(*w,knowledge,sites).kind==build::EventKind::abandoned,"bounded stall");
        // Real integrated world smoke test, not injected knowledge or buildings.
        initialize(*w,42);knowledge={};sites={};Production p{};
        for(unsigned i=0;i<3600;++i){
            discovery::Event re{};build::Event ce{};
            check(build::step(*w,knowledge,sites,p,re,ce,discovery::probe_cadence),"world tick");
            check(re.kind==discovery::EventKind::none||ce.kind==build::EventKind::none,"single task");
        }
        check(discovery::discovered(knowledge)>0&&build::completed(sites)>0,"integrated construction");
        std::cout<<"PASS construction materials, occupancy, progress, abandonment and integration\n";return 0;
    } catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
