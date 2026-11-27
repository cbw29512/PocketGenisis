#pragma once
#include "disease.hpp"
#include "genealogy.hpp"

namespace genesis {
inline bool valid_agent(const Citizen& c) noexcept {
    if(c.x>=width || c.y>=height || c.hunger>100 || c.thirst>100 || c.health>100 ||
       static_cast<unsigned>(c.action)>static_cast<unsigned>(Action::care)||
       static_cast<unsigned>(c.birth_outcome)>static_cast<unsigned>(BirthOutcome::fatal)) return false;
    if(!valid_diseases(c)) return false;
    for(auto trait:c.personality) if(trait>100) return false;
    return static_cast<unsigned>(c.purpose)<=static_cast<unsigned>(Purpose::inventor);
}
inline bool valid_pair(std::uint64_t a,std::uint64_t b) noexcept {
    return (a==1 && b==2) || (a>=3 && a<b);
}
inline constexpr std::size_t live_capacity=descendant_capacity+2;
// Slots are transient: valid only while the active array is not compacted.
inline std::size_t live_slot(const World& w,std::uint64_t id) noexcept {
    if(id==1||id==2) return static_cast<std::size_t>(id-1);
    // Active array is ID sorted; binary lookup avoids scanning every agent.
    std::size_t lo=0,hi=w.descendant_count;
    while(lo<hi) {
        const auto mid=lo+(hi-lo)/2;
        if(w.descendants[mid].id<id) lo=mid+1;
        else hi=mid;
    }
    return lo<w.descendant_count&&w.descendants[lo].id==id?lo+2:live_capacity;
}
inline const Citizen* live_citizen(const World& w,std::uint64_t id) noexcept {
    const auto slot=live_slot(w,id);
    if(slot==live_capacity) return nullptr;
    return slot<2?&w.founders[slot]:&w.descendants[slot-2];
}
inline bool valid_founders(const World& w) noexcept {
    for(std::size_t i=0;i<w.founders.size();++i) {
        const auto& c=w.founders[i];
        if(!valid_agent(c)||c.id!=i+1||c.parent1||c.parent2||c.birth_tick||
           c.purpose==Purpose::none||c.health==0||c.guardian_id||c.guardian_since_tick||
           c.birth_outcome!=BirthOutcome::normal) return false;
    }
    return true;
}
inline bool valid_gestations(const World& w) noexcept {
    if(w.descendant_count>descendant_capacity||w.archive_count>history_capacity) return false;
    // Each gestation reserves an active slot and a future archive identity.
    if(w.pregnancy_count>pregnancy_capacity||
       w.descendant_count+w.pregnancy_count>descendant_capacity||
       w.descendant_count+w.archive_count+w.pregnancy_count>history_capacity) return false;
    std::array<bool,live_capacity> participating{};
    std::uint64_t previous_due=0;
    for(std::size_t i=0;i<w.pregnancy_count;++i) {
        const auto& p=w.pregnancies[i];
        if(!valid_pair(p.parent1,p.parent2)||p.due_tick<=w.tick||
           p.due_tick-w.tick>gestation_ticks||p.due_tick<previous_due) return false;
        const auto* a=live_citizen(w,p.parent1);
        const auto* b=live_citizen(w,p.parent2);
        if(!a||!b||life_stage(*a,w.tick)<LifeStage::adult||life_stage(*b,w.tick)<LifeStage::adult) return false;
        const auto sa=live_slot(w,p.parent1),sb=live_slot(w,p.parent2);
        if(participating[sa]||participating[sb]) return false;
        participating[sa]=participating[sb]=true;
        previous_due=p.due_tick;
    }
    for(std::size_t i=w.pregnancy_count;i<pregnancy_capacity;++i) {
        const auto& p=w.pregnancies[i];
        if(p.parent1||p.parent2||p.due_tick) return false;
    }
    return true;
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
    if(!valid_founders(w)) return false;
    std::uint64_t last_id=2;
    for(std::size_t i=0;i<w.descendant_count;++i) {
        const auto& c=w.descendants[i];
        if(!identity(c,static_cast<std::uint16_t>(i+1))||c.id<=last_id||c.health==0||
           c.birth_outcome==BirthOutcome::fatal) return false;
        if(life_stage(c,w.tick)<LifeStage::adult && c.purpose!=Purpose::none) return false;
        last_id=c.id;
    }
    for(std::size_t i=0;i<w.archive_count;++i) {
        const auto& d=w.archive[i]; const auto& c=d.citizen;
        if(!identity(c,static_cast<std::uint16_t>(archived|(i+1)))||
           d.death_tick<c.birth_tick||d.death_tick>w.tick||
           static_cast<unsigned>(d.cause)<1||static_cast<unsigned>(d.cause)>5||
           (d.cause==DeathCause::old_age?c.health==0:c.health!=0)) return false;
        if(d.cause==DeathCause::disease&&!is_ill(c)) return false;
        if((d.cause==DeathCause::childbirth)!=(c.birth_outcome==BirthOutcome::fatal)||
           (d.cause==DeathCause::childbirth&&d.death_tick!=c.birth_tick)) return false;
        if(i) {
            const auto& previous=w.archive[i-1];
            if(d.death_tick<previous.death_tick ||
               (d.death_tick==previous.death_tick&&c.id<=previous.citizen.id)) return false;
        }
    }
    auto lookup=[&](std::uint64_t id) noexcept -> PersonRecord {
        if(id<3||id>=w.next_citizen_id) return {};
        const auto entry=index[static_cast<std::size_t>(id-3)];
        if(!entry) return {};
        const auto pos=static_cast<std::size_t>((entry&~archived)-1);
        if(entry&archived) return {&w.archive[pos].citizen,true,w.archive[pos].death_tick};
        return {&w.descendants[pos],false,0};
    };
    for(std::size_t i=0;i<w.descendant_count;++i)
        if(!valid_genealogy(w.descendants[i],w.tick,w.next_citizen_id,lookup)) return false;
    for(std::size_t i=0;i<w.archive_count;++i)
        if(!valid_genealogy(w.archive[i].citizen,w.archive[i].death_tick,w.next_citizen_id,lookup)) return false;
    return valid_gestations(w);
}
} // namespace genesis
