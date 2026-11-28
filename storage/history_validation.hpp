#pragma once
#include "../core/population.hpp"

namespace genesis::storage {
// Resident validation assumes the external prefix was independently verified.
template<class Index> inline bool valid_resident_history(const World& w,const Index& index) {
    if(w.seed!=index.seed()||w.descendant_count>descendant_capacity||w.archive_count>history_capacity||
       w.next_citizen_id<3||index.count()>w.next_citizen_id-3||
       w.next_citizen_id-3-index.count()!=w.descendant_count+w.archive_count||
       !valid_founders(w)) return false;
    // All identities in range + no duplicates + exact count imply no missing IDs.
    auto mortal=[&](const Citizen& c) {
        return c.id>=3&&c.id<w.next_citizen_id&&valid_pair(c.parent1,c.parent2)&&c.parent2<c.id&&
               c.birth_tick&&c.birth_tick<=w.tick&&c.lifespan_years>=60&&c.lifespan_years<=80&&valid_agent(c);
    };
    std::uint64_t previous=2;
    for(std::size_t i=0;i<w.descendant_count;++i) {
        const auto& c=w.descendants[i];
        if(!mortal(c)||c.id<=previous||!c.health||c.birth_outcome==BirthOutcome::fatal||index.find(c.id)) return false;
        if(life_stage(c,w.tick)<LifeStage::adult&&c.purpose!=Purpose::none) return false;
        previous=c.id;
    }
    const auto* last=index.last();
    for(std::size_t i=0;i<w.archive_count;++i) {
        const auto& d=w.archive[i];const auto& c=d.citizen;
        if(!mortal(c)||d.death_tick<c.birth_tick||d.death_tick>w.tick||index.find(c.id)||live_citizen(w,c.id)||
           !valid_death_outcome(d)) return false;
        if(last&&(d.death_tick<last->death_tick||(d.death_tick==last->death_tick&&c.id<=last->citizen.id))) return false;
        for(std::size_t j=0;j<i;++j) if(w.archive[j].citizen.id==c.id) return false;
        last=&d;
    }
    auto lookup=[&](std::uint64_t id) -> PersonRecord {
        if(const auto* c=live_citizen(w,id)) return {c,false,0};
        if(const auto* d=index.find(id)) return {&d->citizen,true,d->death_tick};
        for(std::size_t i=0;i<w.archive_count;++i) {
            const auto& d=w.archive[i];if(d.citizen.id==id) return {&d.citizen,true,d.death_tick};
        }
        return {};
    };
    for(std::size_t i=0;i<w.archive_count;++i) {
        const auto& d=w.archive[i];
        if(!valid_genealogy(d.citizen,d.death_tick,w.next_citizen_id,lookup)) return false;
    }
    for(std::size_t i=0;i<w.descendant_count;++i)
        if(!valid_genealogy(w.descendants[i],w.tick,w.next_citizen_id,lookup)) return false;
    return valid_gestations(w);
}
// Full checkpoint audit checks the external records as well as the resident tail.
template<class Index> inline bool valid_history_view(const World& w,const Index& index) {
    if(!valid_resident_history(w,index)) return false;
    auto lookup=[&](std::uint64_t id) -> PersonRecord {
        if(const auto* c=live_citizen(w,id)) return {c,false,0};
        if(const auto* d=index.find(id)) return {&d->citizen,true,d->death_tick};
        for(std::size_t i=0;i<w.archive_count;++i) {
            const auto& d=w.archive[i];if(d.citizen.id==id) return {&d.citizen,true,d.death_tick};
        }
        return {};
    };
    return index.visit([&](const DeathRecord& d) {
        const auto& c=d.citizen;
        return c.id>=3&&c.id<w.next_citizen_id&&valid_pair(c.parent1,c.parent2)&&
               c.parent2<c.id&&c.birth_tick&&c.birth_tick<=w.tick&&
               c.lifespan_years>=60&&c.lifespan_years<=80&&valid_agent(c)&&
               d.death_tick<=w.tick&&valid_genealogy(c,d.death_tick,w.next_citizen_id,lookup);
    });
}
} // namespace genesis::storage
