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
namespace build=genesis::construction;
namespace herd=genesis::livestock;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto w=std::make_unique<World>();auto wild=std::make_unique<wildlife::Wildlife>();
        initialize(*w,42);w->tick=1;
        discovery::State knowledge{};build::State sites{};herd::State animals{};
        auto& a=w->founders[0];auto& b=w->founders[1];
        const auto all=static_cast<discovery::Mask>((1u<<discovery::tech_count)-1u);
        knowledge.known[0]=knowledge.known[1]=all;
        const auto pen=build::location(a);
        sites.sites[build::enclosure].stage=build::Stage::complete;sites.sites[build::enclosure].tile=pen;
        herd::mark_development(*w,sites,*wild);
        for(auto& tile:w->tiles){tile.food=herd::supply_reserve;tile.water=herd::supply_reserve;}
        auto& dest=w->tiles[pen];dest.food=2;dest.water=2;
        auto& source=w->tiles[pen+1];source.food=26;source.water=30;
        wild->tiles[pen+1].count[wildlife::cattle]=2;
        a.x+=2;
        check(herd::seek_work(*w,knowledge,sites,*wild,animals,a)&&a.x==pen%width+1&&a.action==Action::move,"one normal step toward capture");
        check(herd::work(*w,knowledge,sites,*wild,animals,a)==herd::Task::none,"cannot capture while moving");
        a.action=Action::idle;
        check(herd::work(*w,knowledge,sites,*wild,animals,a)==herd::Task::capture&&
              herd::work(*w,knowledge,sites,*wild,animals,a)==herd::Task::capture,"actual local capture");
        check(herd::population(animals)==2&&wild->tiles[pen+1].count[wildlife::cattle]==0,"capture conservation");
        check(herd::work(*w,knowledge,sites,*wild,animals,a)==herd::Task::supply_food&&
              dest.food==4&&source.food==24&&animals.supplied_food==2,"conserved food and source reserve");
        check(herd::work(*w,knowledge,sites,*wild,animals,a)==herd::Task::supply_water&&
              dest.water==6&&source.water==26&&animals.supplied_water==4,"conserved water transfer");
        dest.water=79;
        check(herd::work(*w,knowledge,sites,*wild,animals,a)==herd::Task::supply_water&&
              dest.water==80&&source.water==25,"destination target");
        wild->developed[pen+1]=1;source.food=100;
        check(herd::work(*w,knowledge,sites,*wild,animals,a)==herd::Task::none,"no developed field withdrawals");
        wild->developed[pen+1]=0;
        a.hunger=12;
        check(!herd::seek_work(*w,knowledge,sites,*wild,animals,a)&&
              herd::work(*w,knowledge,sites,*wild,animals,a)==herd::Task::none,"urgent needs before livestock work");
        a.hunger=0;a.x+=1;
        check(herd::work(*w,knowledge,sites,*wild,animals,a)==herd::Task::none,"no remote supplies");
        // Real integrated step selects a handler and commits once after
        // survival/work priority. Existing structures suppress new competing
        // jobs; no harvest or crop work is artificially awarded.
        a.x=pen%width+1;b.x=a.x;b.y=a.y;
        a.action=b.action=Action::idle;a.hunger=a.thirst=b.hunger=b.thirst=0;
        source.food=50;dest.food=4;
        storage::State reserve{};cooking::State cooked{};shelter::State sheltered{};
        settlement::State home{};multi_field::State farms{};irrigation::State water{};
        Production p{};wildlife::Totals ecology{};discovery::Event re{};build::Event ce{};
        // The test world has no other completed buildings; disallow their
        // admission with site states rather than inventing construction work.
        for(unsigned k=0;k<build::site_count;++k) if(k!=build::enclosure) {
            sites.sites[k].stage=build::Stage::abandoned;
            sites.sites[k].tile=static_cast<std::uint16_t>(k);
        }
        const auto before=animals.supplied_food;
        check(herd::step(*w,knowledge,sites,reserve,cooked,sheltered,home,farms,water,*wild,ecology,animals,p,re,ce,discovery::probe_cadence),"integrated tick");
        check(animals.supplied_food==before+4&&dest.food==8&&source.food==46&&herd::valid(animals),"one real delivery per tick");
        w->tick=600;herd::pulse(*w,sites,animals,wildlife::sustainable_candidate);
        check(herd::population(animals)==2&&dest.food==6,"actual livestock feeding");
        std::cout<<"PASS local livestock work planning, transfers and integrated resource debit\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
