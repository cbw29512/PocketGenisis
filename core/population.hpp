#pragma once
#include "world.hpp"

namespace genesis {
inline bool valid_agent(const Citizen& c) noexcept {
    if(c.x>=width || c.y>=height || c.hunger>100 || c.thirst>100 || c.health>100 ||
       static_cast<unsigned>(c.action)>static_cast<unsigned>(Action::care)) return false;
    for(auto trait:c.personality) if(trait>100) return false;
    return static_cast<unsigned>(c.purpose)<=static_cast<unsigned>(Purpose::inventor);
}
inline bool valid_pair(std::uint64_t a,std::uint64_t b) noexcept {
    return (a==1 && b==2) || (a>=3 && a<b);
}
inline const Citizen* live_citizen(const World& w,std::uint64_t id) noexcept {
    if(id==1||id==2) return &w.founders[static_cast<std::size_t>(id-1)];
    // Active array is ID sorted; binary lookup avoids scanning every agent.
    std::size_t lo=0,hi=w.descendant_count;
    while(lo<hi) {
        const auto mid=lo+(hi-lo)/2;
        if(w.descendants[mid].id<id) lo=mid+1;
        else hi=mid;
    }
    return lo<w.descendant_count&&w.descendants[lo].id==id?&w.descendants[lo]:nullptr;
}
inline bool valid_population(const World& w) noexcept {
    if(w.descendant_count>descendant_capacity || w.archive_count>history_capacity ||
       w.descendant_count+w.archive_count>history_capacity ||
       w.next_citizen_id!=w.descendant_count+w.archive_count+3) return false;
    // 0 = missing; positive index = live; high bit = archive. 1 KiB fixed scratch.
    constexpr std::uint16_t archived=0x8000;
    std::array<std::uint16_t,history_capacity> index{};
    auto identity=[&](const Citizen& c,std::uint16_t entry) {
        if(c.id<3||c.id>=w.next_citizen_id||!valid_pair(c.parent1,c.parent2)||
           c.parent2>=c.id||c.birth_tick==0||c.birth_tick>w.tick||
           c.lifespan_years<60||c.lifespan_years>80||!valid_agent(c)) return false;
        auto& slot=index[static_cast<std::size_t>(c.id-3)];
        if(slot) return false;
        slot=entry; return true;
    };
    for(std::size_t i=0;i<w.founders.size();++i) {
        const auto& c=w.founders[i];
        if(!valid_agent(c)||c.id!=i+1||c.parent1||c.parent2||c.birth_tick||
           c.purpose==Purpose::none||c.health==0||c.guardian_id||c.guardian_since_tick) return false;
    }
    std::uint64_t last_id=2;
    for(std::size_t i=0;i<w.descendant_count;++i) {
        const auto& c=w.descendants[i];
        if(!identity(c,static_cast<std::uint16_t>(i+1))||c.id<=last_id||c.health==0) return false;
        if(life_stage(c,w.tick)<LifeStage::adult && c.purpose!=Purpose::none) return false;
        last_id=c.id;
    }
    for(std::size_t i=0;i<w.archive_count;++i) {
        const auto& d=w.archive[i]; const auto& c=d.citizen;
        if(!identity(c,static_cast<std::uint16_t>(archived|(i+1)))||
           d.death_tick<c.birth_tick||d.death_tick>w.tick||
           static_cast<unsigned>(d.cause)<1||static_cast<unsigned>(d.cause)>3||
           (d.cause==DeathCause::old_age?c.health==0:c.health!=0)) return false;
        if(i) {
            const auto& previous=w.archive[i-1];
            if(d.death_tick<previous.death_tick ||
               (d.death_tick==previous.death_tick&&c.id<=previous.citizen.id)) return false;
        }
    }
    auto parent_valid=[&](std::uint64_t id,std::uint64_t birth) {
        if(id<=2) return true; // valid_pair already validates founders.
        const auto entry=index[static_cast<std::size_t>(id-3)];
        if(!entry) return false;
        const auto pos=static_cast<std::size_t>((entry&~archived)-1);
        const bool dead=(entry&archived)!=0;
        const auto& parent=dead?w.archive[pos].citizen:w.descendants[pos];
        return parent.birth_tick<=birth &&
            birth-parent.birth_tick>=18*ticks_per_year &&
            (!dead||w.archive[pos].death_tick>=birth);
    };
    // Guardian provenance is separate from biological lineage and may refer to
    // an archived adult. Check chronology at assignment, not current age.
    auto guardian_valid=[&](const Citizen& c,std::uint64_t observed_tick) {
        if(!c.guardian_id) return c.guardian_since_tick==0;
        const auto id=c.guardian_id, since=c.guardian_since_tick;
        if(id>=w.next_citizen_id||id==c.id||id==c.parent1||id==c.parent2||
           c.parent1<=2||since<c.birth_tick||since>observed_tick||
           since-c.birth_tick>=2*ticks_per_year) return false;
        for(auto parent:{c.parent1,c.parent2}) {
            const auto entry=index[static_cast<std::size_t>(parent-3)];
            if(!(entry&archived)||w.archive[(entry&~archived)-1].death_tick>since) return false;
        }
        if(id<=2) return true;
        const auto entry=index[static_cast<std::size_t>(id-3)];
        if(!entry) return false;
        const auto pos=static_cast<std::size_t>((entry&~archived)-1);
        const bool dead=(entry&archived)!=0;
        const auto& g=dead?w.archive[pos].citizen:w.descendants[pos];
        return g.birth_tick<=since&&since-g.birth_tick>=18*ticks_per_year&&
               (!dead||w.archive[pos].death_tick>=since);
    };
    auto genealogy=[&](const Citizen& c,std::uint64_t observed_tick) {
        return parent_valid(c.parent1,c.birth_tick)&&parent_valid(c.parent2,c.birth_tick)&&
               guardian_valid(c,observed_tick);
    };
    for(std::size_t i=0;i<w.descendant_count;++i)
        if(!genealogy(w.descendants[i],w.tick)) return false;
    for(std::size_t i=0;i<w.archive_count;++i)
        if(!genealogy(w.archive[i].citizen,w.archive[i].death_tick)) return false;
    // Each gestation reserves an active slot and a future archive identity.
    if(w.pregnancy_count>pregnancy_capacity||
       w.descendant_count+w.pregnancy_count>descendant_capacity||
       w.descendant_count+w.archive_count+w.pregnancy_count>history_capacity) return false;
    std::array<bool,history_capacity+3> participating{};
    std::uint64_t previous_due=0;
    for(std::size_t i=0;i<w.pregnancy_count;++i) {
        const auto& p=w.pregnancies[i];
        if(!valid_pair(p.parent1,p.parent2)||p.due_tick<=w.tick||
           p.due_tick-w.tick>gestation_ticks||p.due_tick<previous_due) return false;
        const auto* a=live_citizen(w,p.parent1);
        const auto* b=live_citizen(w,p.parent2);
        if(!a||!b||life_stage(*a,w.tick)<LifeStage::adult||life_stage(*b,w.tick)<LifeStage::adult) return false;
        if(participating[p.parent1]||participating[p.parent2]) return false;
        participating[p.parent1]=participating[p.parent2]=true;
        previous_due=p.due_tick;
    }
    for(std::size_t i=w.pregnancy_count;i<pregnancy_capacity;++i) {
        const auto& p=w.pregnancies[i];
        if(p.parent1||p.parent2||p.due_tick) return false;
    }
    return true;
}
} // namespace genesis
