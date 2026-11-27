#include "../core/cooking.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto w=std::unique_ptr<World>(new World);initialize(*w,42);
        discovery::State research{};construction::State sites{};storage::State reserve{};cooking::State cooked{};
        auto& c=w->founders[0];auto& tile=w->tiles[construction::location(c)];
        const cooking::MealPolicy policy{research,sites,reserve,cooked};
        auto& fire=sites.sites[construction::campfire];
        fire.tile=construction::location(c);
        auto meal=[&](){c.hunger=24;c.thirst=0;c.action=Action::idle;return consume_with_reserve(*w,c,c,false,policy);};
        tile.food=10;tile.wood=10;
        research.known[0]=discovery::tech_bit(discovery::cooking);
        check(meal()&&c.hunger==12&&tile.wood==10,"unbuilt fire");
        fire.stage=construction::Stage::complete;
        research.known[0]=0;
        check(meal()&&c.hunger==12&&tile.wood==10,"unknown cooking");
        research.known[0]=discovery::tech_bit(discovery::cooking);
        check(meal()&&c.hunger==6&&tile.food==7&&tile.wood==9&&cooked.meals==1,"one food one fuel");
        c.hunger=12;c.action=Action::idle;
        check(consume_with_reserve(*w,c,c,false,policy)&&!c.hunger&&tile.wood==9,"no wasted fuel");
        c.thirst=24;c.hunger=24;c.action=Action::idle;
        check(consume_with_reserve(*w,c,c,true,policy)&&c.thirst==12&&c.hunger==24&&tile.wood==9,"water unchanged");
        tile.wood=0;check(meal()&&c.hunger==12&&cooked.meals==1,"no fuel raw meal");
        tile.wood=9;tile.food=0;check(!meal()&&c.hunger==24&&tile.wood==9,"no food no bonus");
        const auto x=c.x;c.x=static_cast<std::uint16_t>(x+3);
        auto& remote=w->tiles[construction::location(c)];remote.food=1;
        check(meal()&&c.hunger==12&&tile.wood==9,"remote meal");c.x=x;
        // Use actual stored stock, including a caregiver feeding an infant.
        auto& store=sites.sites[construction::store];store.stage=construction::Stage::complete;
        store.tile=static_cast<std::uint16_t>(fire.tile+1);
        c.hunger=c.thirst=0;c.action=Action::idle;tile.food=30;
        check(storage::deposit(*w,sites,reserve,c),"reserve deposit");
        tile.food=0;Citizen child{};child.id=3;child.parent1=1;child.parent2=2;child.x=c.x;child.y=c.y;child.hunger=24;
        check(give_care(*w,c,&child,policy)&&child.hunger==6&&c.action==Action::care&&
              reserve.balance==0&&reserve.assisted==1&&tile.wood==8&&cooked.meals==2,"cooked reserve care");
        check(!give_care(*w,c,&child,policy)&&tile.wood==8,"no repeated child meal");
        check(storage::valid(reserve)&&cooking::valid(cooked)&&cooked.relief==12,"accounting");
        initialize(*w,42);research={};sites={};reserve={};cooked={};Production p{};
        for(unsigned i=0;i<3600;++i) {
            discovery::Event re{};construction::Event ce{};
            check(cooking::step(*w,research,sites,reserve,cooked,p,re,ce,discovery::probe_cadence),"integrated tick");
            check(storage::valid(reserve)&&cooking::valid(cooked),"integrated conservation");
        }
        check(discovery::discovered(research)>0&&construction::completed(sites)>0,"integrated progress");
        std::cout<<"PASS cooking: local knowledge, real food/fuel, hunger relief, care, guards and integration\n";return 0;
    } catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
