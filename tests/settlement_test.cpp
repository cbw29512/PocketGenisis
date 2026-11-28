#include "../core/settlement.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto w=std::unique_ptr<World>(new World);initialize(*w,42);
        discovery::State research{};construction::State sites{};storage::State reserve{};
        cooking::State cooked{};shelter::State sheltered{};settlement::State state{};
        auto& a=w->founders[0];auto& b=w->founders[1];
        auto& home=sites.sites[construction::hut];home.tile=construction::location(a);
        auto& stock=w->tiles[home.tile];stock.food=stock.water=50;
        ++a.x;w->tick=120;
        check(!settlement::return_home(*w,sites,a,state),"unfinished home");
        home.stage=construction::Stage::complete;
        stock.food=0;
        check(!settlement::return_home(*w,sites,a,state),"depleted food");
        stock.food=50;stock.water=0;
        check(!settlement::return_home(*w,sites,a,state),"depleted water");
        stock.water=50;a.x+=9;
        check(!settlement::return_home(*w,sites,a,state),"distant home");a.x-=9;
        w->tick=150;
        check(!settlement::return_home(*w,sites,a,state),"outside visit window");
        a.health=80;
        check(settlement::return_home(*w,sites,a,state)&&a.action==Action::move&&
              state.steps==1&&state.arrivals==1,"recovering citizen returns");
        a.action=Action::idle;
        check(!settlement::return_home(*w,sites,a,state),"not pinned at home");
        ++a.x;b.x=a.x;b.y=a.y;a.health=100;w->tick=240;
        a.hunger=11;a.thirst=0;w->tiles[construction::location(a)].food=50;
        const settlement::Work policy{research,sites,reserve,state,discovery::probe_cadence};
        act_survival_with_work(*w,a,nullptr,policy);
        check(a.action==Action::eat&&state.steps==1,"urgent survival precedes home");
        a.hunger=a.thirst=0;a.action=Action::idle;research.job.active=true;research.job.owner=1;
        check(!policy(*w,a)&&state.steps==1,"active research precedes home");
        research.job.active=false;
        // Research is actually eligible at tick240 with the empty knowledge masks.
        // Reserve that action instead of moving home; the fixture must not confuse
        // the higher-priority work reservation with a successful homeward step.
        check(policy(*w,a)&&a.action==Action::idle&&state.steps==1,"new research precedes home");
        const auto all=static_cast<discovery::Mask>((1u<<discovery::tech_count)-1u);
        research.known[0]=research.known[1]=all;
        // With all discoveries known, an affordable unfinished blueprint also
        // takes precedence. Then remove only the construction materials here.
        auto& local=w->tiles[construction::location(a)];
        local.wood=20;local.stone=20;
        check(policy(*w,a)&&a.action==Action::idle&&state.steps==1,"new construction precedes home");
        local.wood=local.stone=0;
        check(policy(*w,a)&&a.action==Action::move&&state.arrivals==2,"real discretionary arrival");
        initialize(*w,42);research={};sites={};reserve={};cooked={};sheltered={};state={};Production p{};
        for(unsigned i=0;i<3600;++i){
            discovery::Event re{};construction::Event ce{};
            check(settlement::step(*w,research,sites,reserve,cooked,sheltered,state,p,re,ce,discovery::probe_cadence),"integrated tick");
            check(storage::valid(reserve)&&cooking::valid(cooked)&&settlement::valid(state),"accounting");
        }
        check(discovery::discovered(research)>0&&construction::completed(sites)>0,"integrated progress");
        std::cout<<"SETTLEMENT_SMOKE steps="<<state.steps<<" arrivals="<<state.arrivals<<'\n';
        return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
