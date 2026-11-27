#define PG_HOUSING_PROBE
#define PG_AGRICULTURE_PROBE
#include "../core/agriculture.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
namespace farm=genesis::agriculture;
namespace build=genesis::construction;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto w=std::unique_ptr<World>(new World);initialize(*w,42);
        discovery::State research{};build::State sites{};farm::State crops{};
        storage::State reserve{};cooking::State cooked{};shelter::State sheltered{};settlement::State home{};
        auto& a=w->founders[0];auto& b=w->founders[1];w->tick=1;
        check(!discovery::eligible(0,discovery::farming),"research prerequisites");
        const auto prereq=static_cast<discovery::Mask>(discovery::tech_bit(discovery::stone_tools)|discovery::tech_bit(discovery::food_storage));
        check(discovery::eligible(prereq,discovery::farming),"research eligible");
        sites.sites[build::hut].stage=build::Stage::complete;
        sites.sites[build::hut].tile=build::location(a);
        sites.sites[build::campfire].stage=build::Stage::complete;
        sites.sites[build::campfire].tile=build::location(a)-1;
        sites.sites[build::store].stage=build::Stage::complete;
        sites.sites[build::store].tile=build::location(a)-2;
        ++a.x;++a.x;b.x=a.x;b.y=a.y;
        auto& stock=w->tiles[build::location(a)];
        stock.food=48;stock.water=50;stock.wood=20;stock.stone=20;
        check(!build::can_start(*w,research,sites,0,build::field),"unknown farming");
        const auto all=static_cast<discovery::Mask>((1u<<discovery::tech_count)-1u);
        research.known[0]=research.known[1]=all;
        stock.food=70;check(!build::can_start(*w,research,sites,0,build::field),"no local demand");
        stock.food=48;
        auto e=build::advance(*w,research,sites);
        check(e.kind==build::EventKind::started&&e.type==build::field&&stock.wood==18&&stock.stone==19,"field construction debit");
        check(farm::work(*w,research,sites,crops,a)==farm::Task::none,"unfinished field");
        for(unsigned i=0;i<20;++i){++w->tick;e=build::advance(*w,research,sites);}
        check(e.kind==build::EventKind::completed&&e.type==build::field&&build::occupied(sites,build::location(a)),"field completed");
        research.known[0]=prereq;check(farm::work(*w,research,sites,crops,a)==farm::Task::none,"unknown farming work");
        research.known[0]=all;
        check(farm::work(*w,research,sites,crops,a)==farm::Task::plant&&stock.food==46&&stock.water==49,"seed and water");
        check(farm::work(*w,research,sites,crops,a)==farm::Task::none,"no instant growth");
        w->tick+=120;stock.water=0;
        check(farm::work(*w,research,sites,crops,a)==farm::Task::none,"dry soil");
        stock.water=49;
        for(unsigned i=0;i<3;++i){
            check(farm::work(*w,research,sites,crops,a)==farm::Task::tend,"timed tending");
            w->tick+=120;
        }
        stock.food=95;
        check(farm::work(*w,research,sites,crops,a)==farm::Task::none,"no overflow harvest");
        stock.food=46;
        check(farm::work(*w,research,sites,crops,a)==farm::Task::harvest&&stock.food==54&&stock.water==46,"actual harvest");
        check(farm::valid(crops)&&crops.plants==1&&crops.harvests==1&&crops.food==8,"crop accounting");
        w->tick=3*ticks_per_year/4;
        check(farm::work(*w,research,sites,crops,a)==farm::Task::none,"winter planting blocked");
        w->tick=ticks_per_year;
        ++a.x;check(farm::work(*w,research,sites,crops,a)==farm::Task::none,"no remote farming");--a.x;
        a.hunger=12;check(farm::work(*w,research,sites,crops,a)==farm::Task::none,"survival first");
        a.hunger=0;
        // At a completed field, a free founder reserves exactly one farming task.
        const farm::Work policy{research,sites,reserve,crops,home,discovery::probe_cadence};
        check(policy(*w,a)&&policy.chosen==a.id,"reserve farm action");
        // Cultivated tile loses wild food renewal, but not water renewal.
        w->tick=59;stock.food=50;stock.water=50;stock.wood=50;
        a.hunger=a.thirst=b.hunger=b.thirst=0;
        auto& wild=w->tiles[build::location(a)-3];wild.food=50;
        Production p{};
        check(detail::step_world_observed(*w,p,valid_population,[](TickPhase) noexcept {},
            NoDiscretionaryWork{},NoReserveFood{},NoRecoveryBonus{},farm::Renewal{sites}),"renewal tick");
        check(stock.food==50&&stock.water==53&&wild.food==52,"only field wild growth suppressed");
        // Isolated crop-locality policy: preserve ordinary movement and allow
        // leaving when water is depleted or urgent survival takes priority.
        initialize(*w,42);research={};sites={};crops={};reserve={};cooked={};sheltered={};home={};
        research.known[0]=research.known[1]=all;
        auto& plot=sites.sites[build::field];plot.stage=build::Stage::complete;plot.tile=build::location(a);
        sites.sites[build::hut].stage=build::Stage::complete;sites.sites[build::hut].tile=plot.tile-1;
        sites.sites[build::campfire].stage=build::Stage::complete;sites.sites[build::campfire].tile=plot.tile-2;
        sites.sites[build::store].stage=build::Stage::complete;sites.sites[build::store].tile=plot.tile-3;
        auto& plot_stock=w->tiles[plot.tile];plot_stock.food=50;plot_stock.water=50;
        w->tick=10;
        check(farm::work(*w,research,sites,crops,a)==farm::Task::plant,"locality crop planted");
        a.x+=3;const auto away=a.x;
        const farm::Work keep_near{research,sites,reserve,crops,home,discovery::probe_cadence};
        check(keep_near(*w,a)&&a.x==away-1&&a.action==Action::move,"return toward growing crop");
        a.x=away;a.action=Action::idle;plot_stock.water=0;
        check(!farm::viable_tending(*w,sites,crops,w->tick),"dry crop not holding farmer");
        // Dry farmland must not suppress ordinary home visits either.
        check(keep_near(*w,a)&&a.action==Action::move&&home.steps==1,
              "dry field allows scheduled home visit");
        a.x=away;a.action=Action::idle;w->tick=20; // outside 0..11 home-return window
        check(!keep_near(*w,a)&&a.x==away,"dry field allows exploration outside home window");
        // Controlled integration: existing farm and knowledge, but all planting,
        // tending, travel, consumption and harvest must occur through real ticks.
        initialize(*w,42);research={};sites={};crops={};reserve={};cooked={};sheltered={};home={};
        research.known[0]=research.known[1]=all;
        a.personality[0]=b.personality[0]=0; // isolate labor from random wandering
        a.x=50;a.y=30;b.x=51;b.y=30;
        sites.sites[build::field].stage=build::Stage::complete;
        sites.sites[build::field].tile=build::location(a);
        sites.sites[build::hut].stage=build::Stage::complete;
        sites.sites[build::hut].tile=build::location(a)+1;
        sites.sites[build::campfire].stage=build::Stage::complete;
        sites.sites[build::campfire].tile=build::location(a)-3;
        sites.sites[build::store].stage=build::Stage::complete;
        sites.sites[build::store].tile=build::location(a)-4;
        auto& controlled=w->tiles[build::location(a)];
        controlled.food=90;controlled.water=100;
        for(unsigned i=0;i<720;++i) {
            discovery::Event re{};build::Event ce{};
            check(farm::step(*w,research,sites,reserve,cooked,sheltered,home,crops,p,re,ce,discovery::probe_cadence),"controlled real tick");
            check(farm::valid(crops),"controlled crop accounting");
        }
        check(crops.harvests>0&&crops.food==crops.harvests*farm::harvest_yield,
              "real tick crop cycle");
        std::cout<<"AGRICULTURE_CONTROLLED harvests="<<crops.harvests<<" food="<<crops.food<<'\n';
        initialize(*w,42);research={};sites={};crops={};reserve={};cooked={};sheltered={};home={};
        unsigned births=0;
        for(unsigned i=0;i<43200;++i) {
            discovery::Event re{};build::Event ce{};
            check(farm::step(*w,research,sites,reserve,cooked,sheltered,home,crops,p,re,ce,discovery::probe_cadence),"integrated tick");
            check(farm::valid(crops)&&storage::valid(reserve)&&cooking::valid(cooked)&&settlement::valid(home),"integrated accounting");
            for(std::size_t j=0;j<w->descendant_count;++j) births+=w->descendants[j].birth_tick==w->tick;
        }
        check(valid_population(*w)&&discovery::discovered(research)>0&&build::completed(sites)>0,"integrated world");
        std::cout<<"AGRICULTURE_SMOKE known="<<discovery::discovered(research)<<" fields="<<(sites.sites[build::field].stage==build::Stage::complete)
                 <<" plants="<<crops.plants<<" tended="<<crops.tended<<" stage="<<unsigned(crops.stage)
                 <<" harvests="<<crops.harvests<<" food="<<crops.food<<" births="<<births
                 <<" plot_food="<<w->tiles[sites.sites[build::field].tile].food
                 <<" plot_water="<<w->tiles[sites.sites[build::field].tile].water
                 <<" founder1_distance="<<std::abs(int(a.x)-int(sites.sites[build::field].tile%width))+std::abs(int(a.y)-int(sites.sites[build::field].tile/width))
                 <<" founder2_distance="<<std::abs(int(b.x)-int(sites.sites[build::field].tile%width))+std::abs(int(b.y)-int(sites.sites[build::field].tile/width))<<'\n';return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
