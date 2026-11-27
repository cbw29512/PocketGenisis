#include "../core/food_store.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto w=std::unique_ptr<World>(new World);initialize(*w,42);
        discovery::State knowledge{};construction::State sites{};storage::State reserve{};
        auto& c=w->founders[0];auto& food=w->tiles[construction::location(c)].food;
        food=30;
        check(!storage::deposit(*w,sites,reserve,c),"unfinished store");
        auto& store=sites.sites[construction::store];
        store.stage=construction::Stage::complete;store.tile=construction::location(c);
        check(storage::deposit(*w,sites,reserve,c)&&food==29&&reserve.balance==1,"actual transfer");
        c.hunger=12;check(!storage::deposit(*w,sites,reserve,c),"urgent need first");
        c.hunger=0;food=24;check(!storage::deposit(*w,sites,reserve,c),"protect local surplus");
        food=0;c.hunger=12;c.thirst=0;
        const auto x=c.x;c.x=static_cast<std::uint16_t>(x+3);
        check(!storage::ReserveFood{sites,reserve}(*w,c),"distance guard");
        c.x=x;
        // Real survival path, not a direct health bonus or double meal.
        act_survival_with_work(*w,c,nullptr,NoDiscretionaryWork{},storage::ReserveFood{sites,reserve});
        check(c.action==Action::eat&&c.hunger==1&&reserve.balance==0&&reserve.withdrawn==1,"reserve consumption before search");
        c.hunger=0;c.action=Action::idle;food=100;
        for(unsigned i=0;i<storage::capacity;++i) check(storage::deposit(*w,sites,reserve,c),"fill reserve");
        check(!storage::deposit(*w,sites,reserve,c),"capacity");
        storage::spoil(3600,reserve);check(reserve.balance==63&&reserve.spoiled==1&&storage::valid(reserve),"spoilage conservation");
        initialize(*w,42);knowledge={};sites={};reserve={};Production p{};
        for(unsigned i=0;i<3600;++i){
            discovery::Event re{};construction::Event ce{};
            check(storage::step(*w,knowledge,sites,reserve,p,re,ce,discovery::probe_cadence),"integrated tick");
            check(storage::valid(reserve),"conservation");
        }
        check(construction::completed(sites)>0&&reserve.deposited>0,"autonomous deposit");
        std::cout<<"PASS food store: actual transfer, scarcity, capacity, spoilage, integrated deposits\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
