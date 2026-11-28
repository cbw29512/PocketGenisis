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
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto w=std::make_unique<World>();initialize(*w,42);w->tick=1;
        discovery::State knowledge{};build::State sites{};
        auto& a=w->founders[0];auto& b=w->founders[1];
        const auto all=static_cast<discovery::Mask>((1u<<discovery::tech_count)-1u);
        knowledge.known[0]=knowledge.known[1]=all;
        // First paid project is an enclosure; all other new admissions closed.
        for(unsigned k=0;k<build::site_count;++k) {
            sites.sites[k].stage=build::Stage::abandoned;sites.sites[k].tile=static_cast<std::uint16_t>(k);
        }
        const auto tile=build::location(a);
        auto& pen=sites.sites[build::enclosure];pen={};
        sites.sites[build::hut].stage=build::Stage::complete;sites.sites[build::hut].tile=tile-1;
        sites.sites[build::field].stage=build::Stage::complete;sites.sites[build::field].tile=tile-2;
        w->descendant_count=1;w->next_citizen_id=4;
        auto& child=w->descendants[0];child.id=3;child.parent1=1;child.parent2=2;child.x=a.x;child.y=a.y;
        auto& stock=w->tiles[tile];stock.food=stock.water=stock.wood=stock.stone=100;
        auto event=build::advance(*w,knowledge,sites);
        check(event.kind==build::EventKind::started&&event.type==build::enclosure&&stock.wood==94&&stock.stone==98,"initial paid work");
        for(unsigned i=0;i<5;++i){++w->tick;build::advance(*w,knowledge,sites);}
        check(pen.work==5,"actual initial labor");
        const auto started=pen.started;
        a.hunger=100;w->tick+=600;
        event=build::advance(*w,knowledge,sites);
        check(event.kind==build::EventKind::abandoned&&pen.work==5,"unfinished project retained");
        b.x=a.x;b.y=a.y;
        check(!build::can_resume_at(*w,knowledge,sites,1,build::enclosure,b),"cooldown");
        w->tick+=build::resume_cooldown;
        knowledge.known[1]=0;
        check(!build::can_resume_at(*w,knowledge,sites,1,build::enclosure,b),"personal knowledge");
        knowledge.known[1]=all;
        stock.food=0;
        check(!build::can_resume_at(*w,knowledge,sites,1,build::enclosure,b),"no local viability");
        stock.food=100;
        b.x+=2;
        check(!build::can_resume_at(*w,knowledge,sites,1,build::enclosure,b),"no remote resumption");
        check(farm_planning::seek(*w,knowledge,sites,b)&&b.x==a.x+1,"purposeful one step return");
        b.x=a.x;b.action=Action::idle;
        check(build::ReserveWork{knowledge,sites,discovery::probe_cadence}(*w,b),"reserve genuine resumed work");
        // Already-paid materials may be gone; resumption does not debit again.
        stock.wood=stock.stone=0;
        event=build::advance(*w,knowledge,sites);
        check(event.kind==build::EventKind::resumed&&event.builder==2&&pen.owner==1&&
              pen.work==5&&pen.started==started&&!pen.finished&&stock.wood==0&&stock.stone==0,"qualified handoff retains labor and materials");
        // Real authoritative ticks finish the remaining work.
        a.hunger=0;a.thirst=0;
        Production p{};
        for(unsigned i=0;i<240&&pen.stage!=build::Stage::complete;++i) {
            discovery::Event re{};build::Event ce{};
            check(build::step(*w,knowledge,sites,p,re,ce,discovery::probe_cadence),"authoritative resumed construction tick");
        }
        check(pen.stage==build::Stage::complete&&pen.work==30&&pen.started==started,"actual completed resumed enclosure");
        check(!build::can_resume_at(*w,knowledge,sites,1,build::enclosure,b),"no repeated completed project");
        std::cout<<"PASS resume paid interrupted construction with qualified local handoff\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
