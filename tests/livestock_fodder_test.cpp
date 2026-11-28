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
namespace herd=genesis::livestock;
namespace build=genesis::construction;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto w=std::make_unique<World>();auto wild=std::make_unique<wildlife::Wildlife>();
        initialize(*w,42);w->tick=21601;
        discovery::State knowledge{};build::State sites{};herd::State animals{};
        auto& a=w->founders[0];auto& b=w->founders[1];
        const auto all=static_cast<discovery::Mask>((1u<<discovery::tech_count)-1u);
        knowledge.known[0]=knowledge.known[1]=all;
        const auto pen=build::location(a);
        for(unsigned k=0;k<build::site_count;++k) {
            sites.sites[k].stage=build::Stage::abandoned;
            sites.sites[k].tile=static_cast<std::uint16_t>(k);
        }
        sites.sites[build::enclosure].stage=build::Stage::complete;
        sites.sites[build::enclosure].tile=pen;
        herd::mark_development(*w,sites,*wild);
        for(auto& t:w->tiles){t.food=24;t.water=100;}
        auto& stock=w->tiles[pen];stock.food=100;
        auto& source=w->tiles[pen+1];source.food=100;
        a.x+=2;
        check(herd::seek_work(*w,knowledge,sites,*wild,animals,a,true)&&a.x==pen%width+1,"travel toward real preseason fodder");
        check(herd::work(*w,knowledge,sites,*wild,animals,a,true)==herd::Task::none,"no work while moving");
        a.action=Action::idle;a.hunger=a.thirst=0;
        check(herd::work(*w,knowledge,sites,*wild,animals,a,true)==herd::Task::store_fodder&&
              source.food==96&&stock.food==100&&animals.fodder.balance==4,"actual separate fodder inventory");
        // Integrated tick: no second job or phantom deposit.
        b.x=a.x;b.y=a.y;b.action=Action::idle;b.hunger=b.thirst=0;
        storage::State reserve{};cooking::State cooked{};shelter::State sheltered{};
        settlement::State home{};multi_field::State farms{};irrigation::State water{};
        Production p{};wildlife::Totals ecology{};discovery::Event re{};build::Event ce{};
        check(herd::step(*w,knowledge,sites,reserve,cooked,sheltered,home,farms,water,*wild,ecology,animals,p,re,ce,discovery::probe_cadence),"real integrated fodder tick");
        check(animals.fodder.balance==8&&source.food==92,"one conserved delivery");
        a.action=Action::idle;
        while(animals.fodder.balance<herd::fodder_target(0)) {
            check(herd::work(*w,knowledge,sites,*wild,animals,a,true)==herd::Task::store_fodder,"preseason storage");
        }
        check(animals.fodder.balance==36&&source.food==64&&herd::valid(animals),"bounded actual reserve");
        wild->tiles[pen+1].count[wildlife::cattle]=2;
        check(herd::work(*w,knowledge,sites,*wild,animals,a,true)==herd::Task::capture&&
              herd::work(*w,knowledge,sites,*wild,animals,a,true)==herd::Task::capture,"actual pair capture");
        stock.food=0;stock.water=100;
        // Compare exactly the same herd and stocks with and without access
        // to the real reserve. All 18 winter pulses, no artificial refills.
        auto old=animals;auto baseline=std::make_unique<World>(*w);
        for(unsigned n=0;n<18;++n) {
            w->tick=32400+600*n;baseline->tick=w->tick;
            herd::pulse(*baseline,sites,old,wildlife::sustainable_candidate);
            herd::pulse(*w,sites,animals,wildlife::sustainable_candidate,true);
        }
        check(herd::population(old)==0&&herd::population(animals)==2&&
              animals.fodder.balance==0&&animals.fodder.released==36&&
              animals.resources.food==36&&stock.water==64&&herd::valid(animals),"winter survival requires actual stored feed");
        herd::pulse(*w,sites,animals,wildlife::sustainable_candidate,true);
        check(animals.resources.food==36,"duplicate pulse blocked");
        w->tick+=600;herd::pulse(*w,sites,animals,wildlife::sustainable_candidate,true);
        check(herd::population(animals)==0&&herd::valid(animals),"exhaustion still causes actual mortality");
        herd::Fodder f{4,4,0};Tile dry{};
        wildlife::Totals tally{};
        check(herd::feed_with_fodder(dry,2,f,tally)==0&&f.balance==4,"no wasted feed without water");
        std::cout<<"PASS conserved seasonal fodder and winter herd survival fixture\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
