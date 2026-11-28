#define PG_HOUSING_PROBE
#define PG_AGRICULTURE_PROBE
#define PG_IRRIGATION_PROBE
#define PG_MULTI_FIELD_PROBE
#define PG_CROP_LOSS_PROBE
#define PG_LIVESTOCK_PROBE
#define PG_SOIL_FERTILITY_PROBE
#define PG_MANURE_PROBE
#define PG_FATIGUE_PROBE
#define PG_REPRODUCTIVE_DRIVE_PROBE
#include "../core/terrain_renewal.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
namespace tp=genesis::terrain_probe;
namespace tr=genesis::terrain_resources;
namespace build=genesis::construction;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto w=std::make_unique<World>();initialize(*w,42);
        auto& a=w->founders[0];auto& b=w->founders[1];
        const auto tile=build::location(a);
        a.fatigue=80;a.hunger=11;a.thirst=0;
        unsigned worked=0;
        const auto job=[&](const World&,Citizen&) noexcept {++worked;return true;};
        const auto food=w->tiles[tile].food;
        act_survival_with_work(*w,a,nullptr,job);
        check(a.action==Action::eat&&w->tiles[tile].food==food-1&&
              a.fatigue==80&&a.rest_intent&&!worked,"urgent food precedes sleep");
        Citizen child{};child.id=3;child.parent1=1;child.parent2=2;
        child.x=a.x;child.y=a.y;child.hunger=12;
        const auto before=w->tiles[tile].food;
        act_survival_with_work(*w,a,&child,job);
        check(a.action==Action::care&&child.hunger==0&&
              w->tiles[tile].food==before-1&&a.fatigue==80&&!worked,"dependent care precedes sleep");
        act_survival_with_work(*w,a,nullptr,job);
        check(a.action==Action::rest&&a.fatigue==76&&a.rest_intent&&
              !discovery::available(a)&&!worked,"rest prevents work");
        // Hunger interrupts sleep without erasing accumulated fatigue.
        a.hunger=11;
        act_survival_with_work(*w,a,nullptr,job);
        check(a.action==Action::eat&&a.fatigue==76&&a.rest_intent,"interrupted sleep resumes");
        a.hunger=a.thirst=0;
        for(unsigned i=0;i<20&&a.rest_intent;++i) {
            ++w->tick;act_survival_with_work(*w,a,nullptr,job);
        }
        check(!a.rest_intent&&a.fatigue<=fatigue::wake_at&&!worked,"hysteresis completes real recovery");
        a.hunger=a.thirst=0;act_survival_with_work(*w,a,nullptr,job);
        check(worked==1&&a.action==Action::idle,"productive work resumes after recovery");
        // A nearby incapacitated partner still receives real assistance.
        a.fatigue=90;a.hunger=a.thirst=0;b.health=10;b.hunger=24;b.thirst=0;b.action=Action::idle;
        act_survival_with_work(*w,a,nullptr,job);
        check(a.action==Action::idle&&assist_founder(*w,a,b)&&a.action==Action::care,
              "emergency partner care retains priority");
        a.fatigue=101;check(!valid_agent(a),"invalid fatigue rejected");a.fatigue=90;
        a.rest_intent=2;check(!valid_agent(a),"invalid intent rejected");
        // Two identical full combined candidate worlds. Compare real
        // fatigue/intent/actions/positions, stock and RNG at each tick.
        initialize(*w,42);auto twin=std::make_unique<World>();initialize(*twin,42);
        auto terrain=std::make_unique<tp::State>();tp::generate(*terrain,42);
        tr::initialize_stocks(*w,*terrain);tr::initialize_stocks(*twin,*terrain);
        struct Context {
            discovery::State knowledge{};build::State sites{};storage::State reserve{};
            cooking::State cooked{};shelter::State sheltered{};settlement::State home{};
            multi_field::State farms{};irrigation::State channels{};livestock::State animals{};
            wildlife::Totals ecology{};Production p{};discovery::Event re{};build::Event ce{};
            wildlife::Wildlife wild{};freshwater::State fish{};
            regional_weather::Cache climate{42};
        };
        auto x=std::make_unique<Context>();auto y=std::make_unique<Context>();
        for(auto* c:{x.get(),y.get()}) {
            wildlife::initialize(c->wild,42);freshwater::initialize(c->fish,*terrain);
        }
        auto tick=[&](World& world,Context& c) {
            const terrain_renewal::Policy policy{*terrain,{c.sites},c.wild,&c.climate};
            return livestock::step_with_renewal(world,c.knowledge,c.sites,c.reserve,c.cooked,c.sheltered,
                  c.home,c.farms,c.channels,c.wild,c.ecology,c.animals,c.p,c.re,c.ce,
                  discovery::probe_cadence,policy,&c.fish,true);
        };
        std::uint64_t rest_ticks=0;
        for(unsigned n=0;n<43200;++n) {
            check(tick(*w,*x)&&tick(*twin,*y),"combined fatigue tick");
            for(unsigned i=0;i<2;++i) {
                const auto& p=w->founders[i];const auto& q=twin->founders[i];
                rest_ticks+=p.action==Action::rest;
                check(p.fatigue==q.fatigue&&p.rest_intent==q.rest_intent&&p.action==q.action&&
                      p.x==q.x&&p.y==q.y&&p.health==q.health&&
                      p.reproductive_drive==q.reproductive_drive,"same-seed fatigue and reproduction replay");
            }
            for(unsigned i=0;i<stream_count;++i)
                check(w->streams[i].state==twin->streams[i].state,"same-seed stream replay");
            check(w->descendant_count==twin->descendant_count&&
                  w->pregnancy_count==twin->pregnancy_count&&
                  w->archive_count==twin->archive_count&&
                  w->next_citizen_id==twin->next_citizen_id,"same-seed lifecycle totals");
            // Check actual geography and life records periodically, not
            // just the final economy counters. Never compare struct padding.
            if(w->tick%600==0) {
                for(std::size_t i=0;i<w->tiles.size();++i) {
                    const auto& u=w->tiles[i];const auto& v=twin->tiles[i];
                    check(u.food==v.food&&u.water==v.water&&u.wood==v.wood&&u.stone==v.stone,
                          "same-seed tile stocks");
                }
                for(std::size_t i=0;i<w->descendant_count;++i) {
                    const auto& u=w->descendants[i];const auto& v=twin->descendants[i];
                    check(u.id==v.id&&u.parent1==v.parent1&&u.parent2==v.parent2&&
                          u.birth_tick==v.birth_tick&&u.x==v.x&&u.y==v.y&&u.health==v.health&&
                          u.fatigue==v.fatigue&&u.rest_intent==v.rest_intent&&
                          u.reproductive_drive==v.reproductive_drive,"same-seed living descendants");
                }
                for(std::size_t i=0;i<w->pregnancy_count;++i) {
                    const auto& u=w->pregnancies[i];const auto& v=twin->pregnancies[i];
                    check(u.parent1==v.parent1&&u.parent2==v.parent2&&u.due_tick==v.due_tick,
                          "same-seed gestation");
                }
            }
        }
        check(rest_ticks>0&&valid_population(*w)&&multi_field::valid(x->farms)&&
              livestock::valid(x->animals)&&freshwater::valid(x->fish),"combined resting and ecosystem accounting");
        check(multi_field::harvests(x->farms)==multi_field::harvests(y->farms)&&
              x->animals.organic.applied==y->animals.organic.applied&&
              freshwater::population(x->fish)==freshwater::population(y->fish),"same-seed economic results");
        std::cout<<"FATIGUE_ECOSYSTEM rest_ticks="<<rest_ticks<<" discoveries="<<discovery::discovered(x->knowledge)
                 <<" crops="<<multi_field::harvests(x->farms)<<" managed="<<livestock::population(x->animals)
                 <<" manure_applied="<<x->animals.organic.applied<<" fish="<<freshwater::population(x->fish)
                 <<" living="<<w->descendant_count+2<<" citizen_bytes="<<sizeof(Citizen)<<'\n';
        std::cout<<"PASS fatigue, sleep interruption, care priority and deterministic combined drive tick\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
