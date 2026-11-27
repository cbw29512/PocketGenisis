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
        // A real care action can draw a reserve unit when the caregiver's tile is empty.
        c.hunger=c.thirst=0;c.action=Action::idle;food=0;
        Citizen child{};child.id=3;child.parent1=1;child.parent2=2;
        child.x=c.x;child.y=c.y;child.hunger=24;
        const auto before=reserve.balance;
        const storage::ReserveFood policy{sites,reserve};
        check(give_care(*w,c,&child,policy),"feed child from reserve");
        check(c.action==Action::care&&child.action==Action::eat&&child.hunger==12&&
              reserve.balance==before-1&&reserve.assisted==1,"one real assisted meal");
        check(!give_care(*w,c,&child,policy)&&reserve.balance==before-1,"no duplicate infant meal");
        // Water remains water; do not substitute a food unit for thirst.
        c.action=Action::idle;child.action=Action::idle;child.thirst=25;
        auto& water=w->tiles[construction::location(c)].water;water=0;
        check(!give_care(*w,c,&child,policy)&&reserve.balance==before-1,"no water fabrication");
        // A distant dependent cannot draw on the caregiver's local reserve.
        child.thirst=0;child.x=static_cast<std::uint16_t>(c.x+3);
        check(!give_care(*w,c,&child,policy)&&reserve.balance==before-1,"distant dependent");
        auto& patient=w->founders[1];patient.x=c.x;patient.y=c.y;
        patient.health=10;patient.hunger=24;patient.thirst=0;patient.action=Action::idle;
        check(assist_founder(*w,c,patient,policy),"incapacitated founder reserve assistance");
        check(patient.hunger==12&&reserve.assisted==2&&reserve.balance==before-2&&storage::valid(reserve),"assisted conservation");
        // Explicitly exercise the secondary food fallback after water is unavailable.
        c.action=Action::idle;c.hunger=24;c.thirst=25;
        for(auto& tile:w->tiles) tile.water=0;
        act_survival_with_work(*w,c,nullptr,NoDiscretionaryWork{},policy);
        check(c.action==Action::eat&&c.hunger==13&&reserve.balance==before-3&&storage::valid(reserve),"secondary need fallback");
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
