#include "../core/shelter.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
static void check(bool ok,const char* why){if(!ok) throw std::runtime_error(why);}
int main(){
    try {
        auto w=std::unique_ptr<World>(new World);initialize(*w,42);
        discovery::State research{};construction::State sites{};storage::State reserve{};
        cooking::State cooked{};shelter::State state{};
        auto& a=w->founders[0];auto& b=w->founders[1];
        auto& hut=sites.sites[construction::hut];hut.tile=construction::location(a);
        a.health=50;update_mortality(*w,shelter::Recovery{sites,state});
        check(a.health==51&&state.extra_recovery==0,"unfinished shelter");
        hut.stage=construction::Stage::complete;
        update_mortality(*w,shelter::Recovery{sites,state});
        check(a.health==53&&state.extra_recovery==1,"local recovery");
        a.action=Action::move;update_mortality(*w,shelter::Recovery{sites,state});
        check(a.health==54&&state.extra_recovery==1,"moving citizen");
        a.action=Action::idle;++a.x;update_mortality(*w,shelter::Recovery{sites,state});
        check(a.health==55&&state.extra_recovery==1,"remote citizen");
        --a.x;a.hunger=100;update_mortality(*w,shelter::Recovery{sites,state});
        check(a.health==54&&state.extra_recovery==1,"starvation unchanged");
        a.hunger=0;a.diseases[0].illness=1;
        update_mortality(*w,shelter::Recovery{sites,state});
        check(a.health==54&&state.extra_recovery==1,"disease unchanged");
        a.diseases[0].illness=0;a.health=99;
        update_mortality(*w,shelter::Recovery{sites,state});
        check(a.health==100&&state.extra_recovery==1,"health capped");
        a.health=b.health=50;b.x=a.x;b.y=a.y;b.action=Action::idle;
        w->descendant_count=1;w->next_citizen_id=4;auto& c=w->descendants[0];
        c.id=3;c.parent1=1;c.parent2=2;c.x=a.x;c.y=a.y;c.health=50;
        update_mortality(*w,shelter::Recovery{sites,state});
        check(a.health==52&&b.health==52&&c.health==51&&state.extra_recovery==3,"two recovery slots");
        c.health=0;update_mortality(*w,shelter::Recovery{sites,state});
        check(w->descendant_count==0&&w->archive_count==1,"no resurrection");
        // Natural age limit remains authoritative even in a shelter.
        w->descendant_count=1;w->next_citizen_id=5;auto& elder=w->descendants[0];
        elder.id=4;elder.parent1=1;elder.parent2=2;elder.x=a.x;elder.y=a.y;elder.health=50;
        w->tick=std::uint64_t(elder.lifespan_years)*ticks_per_year;
        update_mortality(*w,shelter::Recovery{sites,state});
        check(w->descendant_count==0&&w->archive_count==2&&w->archive[1].cause==DeathCause::old_age,"age limit unchanged");
        initialize(*w,42);research={};sites={};reserve={};cooked={};state={};Production p{};
        for(unsigned i=0;i<3600;++i){
            discovery::Event re{};construction::Event ce{};
            check(shelter::step(*w,research,sites,reserve,cooked,state,p,re,ce,discovery::probe_cadence),"integrated tick");
            check(storage::valid(reserve)&&cooking::valid(cooked),"resource accounting");
        }
        check(discovery::discovered(research)>0&&construction::completed(sites)>0,"integrated progress");
        std::cout<<"PASS shelter: local bounded recovery, mortality guards and integrated world\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
