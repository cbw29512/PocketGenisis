#pragma once
#include "population.hpp"
#include "birth_health.hpp"
#include <algorithm>
#include <cstdlib>

namespace genesis {
inline bool eligible_parent(const Citizen& c,std::uint64_t tick) noexcept {
    return life_stage(c,tick)==LifeStage::adult&&c.health>=50&&c.hunger<=24&&c.thirst<=24;
}
inline const Citizen& active_at(const World& w,std::size_t i) noexcept {
    return i<2?w.founders[i]:w.descendants[i-2];
}
inline void attempt_conception(World& w) noexcept {
    if(w.tick%600||w.pregnancy_count>=pregnancy_capacity||
       w.descendant_count+w.pregnancy_count>=descendant_capacity||
       w.descendant_count+w.archive_count+w.pregnancy_count>=history_capacity||
       w.tick>std::numeric_limits<std::uint64_t>::max()-gestation_ticks) return;
    // Parents cannot participate in two simultaneous gestations.
    std::array<bool,live_capacity> busy{};
    for(std::size_t i=0;i<w.pregnancy_count;++i) {
        const auto& p=w.pregnancies[i];
        const auto a=live_slot(w,p.parent1),b=live_slot(w,p.parent2);
        if(a==live_capacity||b==live_capacity) return;
        busy[a]=busy[b]=true;
    }
    // Reservoir sampling: every locally eligible pair has equal selection chance.
    std::uint32_t count=0,roll{};
    Pregnancy selected{};
    const auto n=w.descendant_count+2;
    for(std::size_t i=0;i<n;++i) {
        const auto& a=active_at(w,i);
        if(busy[i]||!eligible_parent(a,w.tick)) continue;
        for(std::size_t j=i+1;j<n;++j) {
            const auto& b=active_at(w,j);
            if(busy[j]||!valid_pair(a.id,b.id)||!eligible_parent(b,w.tick)||
               std::abs(int(a.x)-int(b.x))+std::abs(int(a.y)-int(b.y))>2) continue;
            ++count;
            if(count==1||(w.streams[reproduction].bounded(count,roll)&&roll==0))
                selected={a.id,b.id,w.tick+gestation_ticks};
        }
    }
    if(count&&w.streams[reproduction].bounded(4,roll)&&roll==0)
        w.pregnancies[w.pregnancy_count++]=selected;
}
inline void deliver(World& w,const Citizen& a,const Citizen& b) noexcept {
    // Parents are read from existing slots; fixed array never reallocates.
    auto& child=w.descendants[w.descendant_count++];
    child=Citizen{};
    child.id=w.next_citizen_id++;
    child.parent1=a.id;child.parent2=b.id;child.birth_tick=w.tick;
    child.x=a.x;child.y=a.y;
    for(std::size_t i=0;i<child.personality.size();++i) {
        std::uint32_t roll{};
        w.streams[reproduction].bounded(21,roll);
        const int mean=(a.personality[i]+b.personality[i])/2;
        child.personality[i]=static_cast<std::uint8_t>(std::clamp(mean+static_cast<int>(roll)-10,0,100));
    }
    std::uint32_t roll{};
    w.streams[reproduction].bounded(21,roll);
    child.lifespan_years=static_cast<std::uint8_t>(60+roll);
    inherit_resistance(w,child,a,b);
    apply_birth_health(w,child,a,b);
    if(child.birth_outcome==BirthOutcome::fatal) {
        archive_death(w,child,DeathCause::childbirth);
        child=Citizen{};
        --w.descendant_count;
    }
}
inline void update_lifecycle(World& w) noexcept {
    for(std::size_t i=0;i<w.descendant_count;++i) {
        auto& c=w.descendants[i];
        if(c.purpose==Purpose::none&&life_stage(c,w.tick)>=LifeStage::adult)
            c.purpose=choose_purpose(c);
    }
    // Preserve due-tick order while resolving births/losses after mortality.
    std::size_t write=0;
    bool resolved=false;
    for(std::size_t i=0;i<w.pregnancy_count;++i) {
        const auto p=w.pregnancies[i];
        const auto* a=live_citizen(w,p.parent1);
        const auto* b=live_citizen(w,p.parent2);
        if(!a||!b) {
            if(w.lost_gestations<std::numeric_limits<std::uint64_t>::max()) ++w.lost_gestations;
            resolved=true;
        } else if(w.tick==p.due_tick) {
            deliver(w,*a,*b);
            resolved=true;
        } else {
            w.pregnancies[write++]=p;
        }
    }
    for(std::size_t i=write;i<w.pregnancy_count;++i) w.pregnancies[i]={};
    w.pregnancy_count=write;
    // Keep the existing one-roll-per-opportunity budget, even with many families.
    if(!resolved) attempt_conception(w);
}
} // namespace genesis
