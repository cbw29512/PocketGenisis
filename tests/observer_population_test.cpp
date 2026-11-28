#include "../benchmarks/elecrow/observer_population.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace genesis;
using namespace genesis::observer;
static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(){
    try{
        auto w=std::make_unique<World>();initialize(*w,42);
        auto p=population_stats(*w);
        check(p.founders==2&&!p.descendants&&!p.stages[3],"founders not mortal adults");
        check(death_pages(0)==1&&!death_at(*w,0,0),"empty archive");
        w->tick=9*ticks_per_year;
        w->descendant_count=2;
        w->descendants[0].id=3;w->descendants[0].birth_tick=w->tick-100;
        w->descendants[1].id=4;w->descendants[1].birth_tick=ticks_per_year;
        w->archive_count=7;w->lost_gestations=3;w->pregnancy_count=1;
        for(unsigned i=0;i<7;++i){
            w->archive[i].citizen.id=5+i;
            w->archive[i].cause=static_cast<DeathCause>(i<6?i+1:1);
        }
        p=population_stats(*w);
        check(p.founders==2&&p.descendants==2&&p.stages[0]==1&&p.stages[1]==1&&!p.stages[3],"mortal age stages");
        check(p.deaths==7&&p.causes[0]==2&&p.causes[1]==1&&p.causes[2]==1&&p.causes[3]==1&&p.causes[4]==1&&p.causes[5]==1,"all cause buckets");
        check(p.live_births==8&&p.fatal_births==1&&p.lost_gestations==3&&p.pregnancies==1,"distinct birth outcomes");
        check(death_pages(7)==2&&death_at(*w,0,0)->citizen.id==11&&death_at(*w,1,1)->citizen.id==5,"newest first");
        check(!death_at(*w,1,2)&&!death_at(*w,2,0)&&!death_at(*w,0,5),"bounded archive page");
        w->archive[0].cause=static_cast<DeathCause>(255);
        check(population_stats(*w).unknown==1&&cause_label(w->archive[0].cause)[0]=='U',"unknown cause explicit");
        std::cout<<"PASS founder demographics, birth accounting and death history projection\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
