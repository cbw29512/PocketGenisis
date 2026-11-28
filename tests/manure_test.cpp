#define PG_HOUSING_PROBE
#define PG_AGRICULTURE_PROBE
#define PG_IRRIGATION_PROBE
#define PG_MULTI_FIELD_PROBE
#define PG_CROP_LOSS_PROBE
#define PG_LIVESTOCK_PROBE
#define PG_SOIL_FERTILITY_PROBE
#define PG_MANURE_PROBE
#include "../core/livestock_integration.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
namespace build=genesis::construction;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto w=std::make_unique<World>();initialize(*w,42);
        auto wild=std::make_unique<wildlife::Wildlife>();
        auto& a=w->founders[0];auto& b=w->founders[1];
        const auto pen=build::location(a);
        const auto field=static_cast<std::uint16_t>(pen+2);
        discovery::State knowledge{};build::State sites{};multi_field::State farms{};
        livestock::State animals{};
        for(unsigned k=0;k<build::site_count;++k) {
            sites.sites[k].stage=build::Stage::abandoned;
            sites.sites[k].tile=static_cast<std::uint16_t>(k);
        }
        sites.sites[build::enclosure].stage=build::Stage::complete;
        sites.sites[build::enclosure].tile=pen;
        sites.sites[build::field].stage=build::Stage::complete;
        sites.sites[build::field].tile=field;
        auto& crop=farms.crops[0];
        // Controlled previous cultivation ledger: ten completed crops.
        crop.plants=10;crop.tended=30;crop.water=40;crop.harvests=10;crop.food=60;
        crop.nutrients.fertility=20;crop.nutrients.spent=80;
        check(multi_field::valid(farms),"valid exhausted field");
        auto& group=animals.groups[0];group.captured=4;group.count=4;
        w->tiles[pen].food=100;w->tiles[pen].water=100;
        w->tiles[field].food=50;w->tiles[field].water=100;
        w->tick=600;
        livestock::pulse(*w,sites,animals,wildlife::sustainable_candidate,true);
        check(animals.organic.feed==4&&animals.organic.produced==2&&
              animals.organic.balance==2&&w->tiles[pen].food==96&&
              w->tiles[pen].water==96&&livestock::valid(animals),"actual feed produces byproduct");
        livestock::pulse(*w,sites,animals,wildlife::sustainable_candidate,true);
        check(animals.organic.produced==2,"no duplicate feed byproduct");
        check(!manure::work(*w,knowledge,sites,farms,animals.organic,a,manure::Task::collect),"unlearned collection denied");
        const auto all=static_cast<discovery::Mask>((1u<<discovery::tech_count)-1u);
        knowledge.known[0]=knowledge.known[1]=all;
        check(!discovery::eligible(discovery::tech_bit(discovery::farming),discovery::soil_amendment),"husbandry prerequisite");
        check(!manure::work(*w,knowledge,sites,farms,animals.organic,a,manure::Task::apply),"no carried material or remote application");
        // Run the actual combined tick. Founders are together at the
        // pen and all other construction/research is inactive.
        b.x=a.x;b.y=a.y;
        storage::State reserve{};cooking::State cooked{};shelter::State sheltered{};
        settlement::State home{};irrigation::State channels{};wildlife::Totals ecology{};
        Production p{};discovery::Event re{};build::Event ce{};
        check(livestock::step(*w,knowledge,sites,reserve,cooked,sheltered,home,farms,channels,
              *wild,ecology,animals,p,re,ce,discovery::probe_cadence),"authoritative collection tick");
        check(animals.organic.balance==0&&animals.organic.carried[0]==2&&
              crop.nutrients.fertility==20,"collection only, no remote fertility");
        for(unsigned n=0;n<10&&!animals.organic.applied;++n)
            check(livestock::step(*w,knowledge,sites,reserve,cooked,sheltered,home,farms,channels,
                  *wild,ecology,animals,p,re,ce,discovery::probe_cadence),"authoritative travel and application");
        check(animals.organic.applied>0&&crop.nutrients.amended==4*animals.organic.applied&&
              crop.nutrients.fertility>=24&&livestock::valid(animals)&&multi_field::valid(farms),
              "actual connected livestock-to-soil cycle");
        // No phantom production from an unfed herd, and cap overflow
        // remains explicitly accounted rather than becoming new stock.
        manure::Store bounded{};manure::record_feed(bounded,200);
        check(bounded.balance==64&&bounded.lost==36&&manure::valid(bounded),"bounded byproduct ledger");
        std::cout<<"MANURE_CYCLE fed="<<animals.organic.feed<<" produced="<<animals.organic.produced
                 <<" applied="<<animals.organic.applied<<" fertility="<<unsigned(crop.nutrients.fertility)<<'\n';
        std::cout<<"PASS learned livestock byproduct collection and local soil amendment\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
