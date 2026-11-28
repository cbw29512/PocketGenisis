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
        auto w=std::unique_ptr<World>(new World);auto wild=std::unique_ptr<wildlife::Wildlife>(new wildlife::Wildlife{});
        initialize(*w,42);
        discovery::State knowledge{};build::State sites{};herd::State animals{};
        auto& a=w->founders[0];w->tick=1;
        const auto all=static_cast<discovery::Mask>((1u<<discovery::tech_count)-1u);
        check(!discovery::eligible(0,discovery::husbandry)&&
              discovery::eligible(discovery::tech_bit(discovery::farming),discovery::husbandry),"knowledge prerequisite");
        knowledge.known[0]=knowledge.known[1]=all;
        const auto origin=build::location(a);
        sites.sites[build::hut].stage=build::Stage::complete;sites.sites[build::hut].tile=origin-1;
        sites.sites[build::field].stage=build::Stage::complete;sites.sites[build::field].tile=origin-2;
        sites.sites[build::campfire].stage=build::Stage::complete;sites.sites[build::campfire].tile=origin-3;
        sites.sites[build::store].stage=build::Stage::complete;sites.sites[build::store].tile=origin-4;
        auto& stock=w->tiles[origin];stock.food=stock.water=stock.wood=stock.stone=50;
        check(!build::can_start(*w,knowledge,sites,0,build::enclosure),"no family demand");
        w->descendant_count=1;w->next_citizen_id=4;auto& child=w->descendants[0];child.id=3;child.parent1=1;child.parent2=2;child.x=a.x;child.y=a.y;
        auto event=build::advance(*w,knowledge,sites);
        check(event.kind==build::EventKind::started&&event.type==build::enclosure&&stock.wood==44&&stock.stone==48,"shared construction debit");
        ++a.x;auto& source=wild->tiles[build::location(a)];
        source.count[wildlife::cattle]=2;
        check(herd::work(*w,knowledge,sites,*wild,animals,a)==herd::Task::none,"unfinished pen");
        --a.x;
        for(unsigned i=0;i<30;++i){++w->tick;event=build::advance(*w,knowledge,sites);}
        check(event.kind==build::EventKind::completed&&event.type==build::enclosure,"enclosure completed");
        herd::mark_development(*w,sites,*wild);
        check(wild->developed[origin]&&!wild->developed[origin+1],"actual development");
        ++a.x;knowledge.known[0]=0;
        check(herd::work(*w,knowledge,sites,*wild,animals,a)==herd::Task::none,"no personal knowledge");
        knowledge.known[0]=all;
        check(herd::work(*w,knowledge,sites,*wild,animals,a)==herd::Task::capture&&
              herd::work(*w,knowledge,sites,*wild,animals,a)==herd::Task::capture&&
              source.count[wildlife::cattle]==0&&herd::population(animals)==2,"conserved wild capture");
        w->tiles[build::location(a)].food=w->tiles[build::location(a)].water=herd::supply_reserve;
        check(herd::work(*w,knowledge,sites,*wild,animals,a)==herd::Task::none,"no free animals");
        w->tick=600;herd::pulse(*w,sites,animals);
        check(animals.groups[0].count==3&&stock.food==47&&stock.water==47&&herd::valid(animals),"resource limited birth");
        herd::pulse(*w,sites,animals);check(stock.food==47,"no duplicate pulse");
        w->tick=1200;herd::pulse(*w,sites,animals);
        check(animals.groups[0].count==4,"second birth");
        --a.x;
        check(herd::work(*w,knowledge,sites,*wild,animals,a)==herd::Task::harvest&&
              animals.groups[0].count==3&&wild->tiles[origin].meat==8&&herd::valid(animals),"real meat and breeding floor");
        check(herd::work(*w,knowledge,sites,*wild,animals,a)==herd::Task::none,"preserve breeding group");
        a.hunger=24;wildlife::Totals meals{};wildlife::hunt(a,wild->tiles[origin],w->tick,meals,true);
        check(a.hunger==12&&meals.meals==1&&wild->tiles[origin].meat==7,"existing meat consumption");
        stock.food=0;w->tick=1260;herd::pulse(*w,sites,animals);
        check(!herd::population(animals)&&animals.groups[0].deaths==3&&herd::valid(animals),"starvation accounted");
        // Separate unforced integration with genuine initial wild animals and no injected technology.
        initialize(*w,42);wildlife::initialize(*wild,w->seed);knowledge={};sites={};animals={};
        const auto initial=wildlife::population(*wild);wildlife::Totals ecology{};
        storage::State reserve{};cooking::State cooked{};shelter::State sheltered{};settlement::State home{};
        multi_field::State farms{};irrigation::State water{};Production p{};
        for(unsigned i=0;i<43200;++i){
            discovery::Event re{};build::Event ce{};
            check(herd::step(*w,knowledge,sites,reserve,cooked,sheltered,home,farms,water,*wild,ecology,animals,p,re,ce,discovery::probe_cadence),"real integrated tick");
            check(herd::valid(animals)&&multi_field::valid(farms)&&storage::valid(reserve),"local conservation");
            if(ce.kind!=build::EventKind::none) std::cout<<"LIVESTOCK_BUILD tick="<<w->tick
                <<" type="<<unsigned(ce.type)<<" event="<<unsigned(ce.kind)<<" tile="<<ce.tile<<'\n';
            if(w->tick%10800==0) {
                const auto& plot=sites.sites[build::field];const auto& crop=farms.crops[0];
                const auto& stock=w->tiles[plot.tile];
                const auto& pen=sites.sites[build::enclosure];
                const auto& pen_stock=w->tiles[pen.tile];
                std::cout<<"LIVESTOCK_QUARTER tick="<<w->tick<<" field="<<unsigned(plot.stage)
                    <<" plants="<<crop.plants<<" tended="<<crop.tended<<" stage="<<unsigned(crop.stage)
                    <<" distributed="<<crop.distributed<<" harvests="<<crop.harvests
                    <<" crop_losses="<<crop.drought_losses+crop.spoilage_losses
                    <<" food="<<stock.food<<" water="<<stock.water
                    <<" distance1="<<std::abs(int(w->founders[0].x)-int(plot.tile%width))+std::abs(int(w->founders[0].y)-int(plot.tile/width))
                    <<" distance2="<<std::abs(int(w->founders[1].x)-int(plot.tile%width))+std::abs(int(w->founders[1].y)-int(plot.tile/width))
                    <<" pen="<<unsigned(pen.stage)<<" pen_food="<<pen_stock.food<<" pen_water="<<pen_stock.water
                    <<" supplied_food="<<animals.supplied_food<<" supplied_water="<<animals.supplied_water
                    <<" fodder="<<animals.fodder.balance<<" fodder_deposited="<<animals.fodder.deposited
                    <<" fodder_released="<<animals.fodder.released
                    <<" managed="<<herd::population(animals)<<'\n';
            }
        }
        check(wildlife::valid(*wild)&&valid_population(*w)&&
              initial+ecology.births==wildlife::population(*wild)+ecology.deaths+herd::captures(animals),"wild transfer accounting");
        std::uint64_t remaining_meat=0;
        for(const auto& t:wild->tiles) remaining_meat+=t.meat;
        check(ecology.meat_created+animals.meat_produced==remaining_meat+ecology.meals+ecology.spoiled,"combined meat conservation");
        std::cout<<"LIVESTOCK_ECOLOGY initial="<<initial<<" births="<<ecology.births
                 <<" deaths="<<ecology.deaths<<" shortage="
                 <<ecology.deaths-ecology.predation-ecology.hunted-ecology.displaced_deaths
                 <<" predation="<<ecology.predation<<" hunted="<<ecology.hunted
                 <<" meals="<<ecology.meals<<" hunted_yield="<<ecology.meat_created
                 <<" meat_remaining="<<remaining_meat<<" spoiled="<<ecology.spoiled
                 <<" displaced="<<ecology.displaced_deaths<<" food="<<ecology.food
                 <<" water="<<ecology.water<<" managed_births="<<animals.groups[0].births+animals.groups[1].births
                 <<" managed_deaths="<<animals.groups[0].deaths+animals.groups[1].deaths<<'\n';
        std::cout<<"LIVESTOCK_SMOKE discovered="<<discovery::discovered(knowledge)
                 <<" pen="<<(sites.sites[build::enclosure].stage==build::Stage::complete)
                 <<" captures="<<herd::captures(animals)<<" living_animals="<<herd::population(animals)
                 <<" meat="<<animals.meat_produced<<" wild="<<wildlife::population(*wild)
                 <<" harvests="<<multi_field::harvests(farms)<<" living="<<w->descendant_count+2<<'\n';return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
