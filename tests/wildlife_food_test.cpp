#define PG_HOUSING_PROBE
#define PG_AGRICULTURE_PROBE
#define PG_IRRIGATION_PROBE
#define PG_MULTI_FIELD_PROBE
#define PG_CROP_LOSS_PROBE
#define PG_LIVESTOCK_PROBE
#include "../core/livestock_integration.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
namespace wild=genesis::wildlife;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto w=std::make_unique<World>();auto animals=std::make_unique<wild::Wildlife>();
        initialize(*w,42);
        discovery::State research{};construction::State sites{};storage::State reserve{};
        cooking::State cooked{};wild::Totals totals{};
        const wild::MealPolicy food{{research,sites,reserve,cooked},*animals,totals};
        auto& a=w->founders[0];auto& b=w->founders[1];
        const auto i=construction::location(a);auto& tile=animals->tiles[i];auto& stock=w->tiles[i];
        tile.count[wild::deer]=2;stock.food=1;stock.water=100;
        a.hunger=11;a.thirst=0;
        act_survival_with_work(*w,a,nullptr,NoDiscretionaryWork{},food);
        check(a.action==Action::eat&&a.hunger==0&&stock.food==1&&tile.count[wild::deer]==1&&
              totals.hunted==1&&tile.meat==5&&totals.meals==1,"actual urgent hunger hunts before farming");
        a.hunger=11;
        act_survival_with_work(*w,a,nullptr,NoDiscretionaryWork{},food);
        check(tile.count[wild::deer]==1&&tile.meat==4&&totals.hunted==1&&totals.meals==2,"consume existing meat first");
        a.hunger=11;a.thirst=11;
        act_survival_with_work(*w,a,nullptr,NoDiscretionaryWork{},food);
        check(a.action==Action::drink&&tile.meat==4&&stock.water==99,"hydration priority");
        // Child may receive existing food but cannot personally hunt.
        Citizen child{};child.id=3;child.birth_tick=1;child.x=a.x;child.y=a.y;child.hunger=12;
        a.action=Action::idle;
        check(consume_with_reserve(*w,child,a,false,food)&&tile.meat==3&&totals.meals==3,"actual stored meat for dependent");
        tile.meat=0;stock.food=0;child.hunger=12;child.action=Action::idle;
        check(!consume_with_reserve(*w,child,a,false,food)&&tile.count[wild::deer]==1,"care cannot silently hunt");
        animals->developed[i]=1;a.hunger=12;a.action=Action::idle;
        check(!consume_with_reserve(*w,a,a,false,food)&&tile.count[wild::deer]==1,"no developed-land hunting");
        // Isolate a real nearby prey target: search and travel do not kill.
        for(auto& t:w->tiles){t.food=0;t.water=100;}
        animals->tiles.fill(wild::WildlifeTile{});animals->developed.fill(0);
        animals->tiles[i+2].count[wild::rabbit]=2;
        a.hunger=11;a.thirst=0;a.action=Action::idle;
        act_survival_with_work(*w,a,nullptr,NoDiscretionaryWork{},food);
        check(a.action==Action::move&&a.x==51&&animals->tiles[i+2].count[wild::rabbit]==2,"bounded prey seeking ordinary step");
        // Use a fresh integrated world with no learned technology and an
        // actual prey source at a hungry founder's position.
        initialize(*w,42);animals->tiles.fill(wild::WildlifeTile{});animals->developed.fill(0);totals={};
        animals->tiles[i].count[wild::rabbit]=2;w->tiles[i].food=1;
        a.hunger=11;a.thirst=0;b.hunger=b.thirst=0;
        shelter::State sheltered{};settlement::State home{};multi_field::State farms{};
        irrigation::State water{};livestock::State herd{};Production p{};
        discovery::Event re{};construction::Event ce{};
        check(livestock::step(*w,research,sites,reserve,cooked,sheltered,home,farms,water,*animals,totals,herd,p,re,ce,discovery::probe_cadence),"actual combined food tick");
        check(totals.hunted==1&&totals.deaths==1&&totals.meals==1&&totals.meat_created==2&&
              animals->tiles[i].meat==1&&wild::population(*animals)==1&&a.action==Action::eat&&
              !discovery::discovered(research),"integrated hunting, no duplicate action or invented resources");
        std::cout<<"PASS authoritative hunting and local meat consumption\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
