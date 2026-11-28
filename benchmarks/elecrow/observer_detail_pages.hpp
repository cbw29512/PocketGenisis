#pragma once
#include "observer_population.hpp"
#include "../../core/integrated_world.hpp"
namespace genesis::observer {
// Text projections are kept apart from LCD painting and simulation logic.
// Each page uses the same cached row writer as the world overview.
template<class Line>
void people_page(const integrated::State& state,const Line& line){
    const auto p=population_stats(state.world);
    line(0,48,90,58,"Population and family");
    line(1,48,126,30,"Living: %u",unsigned(p.founders+p.descendants));
    line(2,48,150,30,"Immortal founders: %u",unsigned(p.founders));
    line(3,48,174,30,"Living descendants: %u",unsigned(p.descendants));
    line(4,48,198,30,"Live births: %u",unsigned(p.live_births));
    line(5,48,222,30,"Fatal births: %u",unsigned(p.fatal_births));
    line(6,48,246,30,"Pregnancies: %u",unsigned(p.pregnancies));
    line(7,48,270,30,"Lost gestations: %llu",(unsigned long long)p.lost_gestations);
    line(8,420,126,27,"Mortal life stages");
    line(9,420,150,27,"Infants: %u",p.stages[0]);
    line(10,420,174,27,"Children: %u",p.stages[1]);
    line(11,420,198,27,"Teens: %u",p.stages[2]);
    line(12,420,222,27,"Adults: %u",p.stages[3]);
    line(13,420,246,27,"Elders: %u",p.stages[4]);
    line(14,420,270,27,"Death records: %u",unsigned(p.deaths));
}
template<class Line>
void deaths_page(const integrated::State& state,unsigned page,const Line& line){
    const auto p=population_stats(state.world);
    line(0,48,90,30,"Death records: %u",unsigned(p.deaths));
    for(unsigned i=0;i<death_cause_count;++i)
        line(i+1,48,126+int(i)*23,30,"%s: %u",death_labels[i],p.causes[i]);
    line(7,48,264,30,"Unknown: %u",unsigned(p.unknown));
    line(8,420,90,27,"Newest first %u/%u",page+1,unsigned(death_pages(p.deaths)));
    for(unsigned i=0;i<deaths_per_page;++i){
        const auto* d=death_at(state.world,page,i);
        const int y=112+int(i)*30;
        if(d){
            const auto age=d->death_tick>=d->citizen.birth_tick?d->death_tick-d->citizen.birth_tick:0;
            line(9+i*2,420,y,27,"Pixel %llu",(unsigned long long)d->citizen.id);
            line(10+i*2,420,y+15,27,"%s %lluy%llum",cause_label(d->cause),
                (unsigned long long)(age/ticks_per_year),
                (unsigned long long)(age%ticks_per_year/(ticks_per_year/12)));
        }else{
            line(9+i*2,420,y,27,"%s","");
            line(10+i*2,420,y+15,27,"%s","");
        }
    }
}
template<class Line>
void progress_page(const integrated::State& state,const Line& line){
    line(0,48,90,30,"Actual discoveries");
    line(1,420,90,27,"Completed structures");
    unsigned row=0;
    for(unsigned i=0;i<discovery::tech_count;++i){
        const auto& record=state.knowledge.first[i];
        // Do not expose undiscovered technologies as achievements.
        if(record.founder)line(2+row,48,112+int(row)*20,30,"%s Y%llu",
            discovery::rules[i].name,(unsigned long long)(record.tick/ticks_per_year+1)),++row;
    }
    if(!row)line(2,48,112,30,"None yet");
    std::array<unsigned,construction::kind_count> counts{};
    for(unsigned i=0;i<construction::site_count;++i)
        if(state.sites.sites[i].stage==construction::Stage::complete)++counts[construction::kind_at(i)];
    for(unsigned i=0;i<construction::kind_count;++i)
        line(12+i,420,112+int(i)*23,27,"%s: %u",construction::blueprints[i].name,counts[i]);
    line(18,420,264,27,"Total: %u",construction::completed(state.sites));
}
}
