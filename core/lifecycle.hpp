#pragma once
#include "population.hpp"
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
    if(w.tick%600||w.descendant_count>=descendant_capacity||
       w.descendant_count+w.archive_count>=history_capacity||
       w.tick>std::numeric_limits<std::uint64_t>::max()-gestation_ticks) return;
    // Reservoir sampling: every locally eligible pair has equal selection chance.
    std::uint32_t count=0,roll{};
    Pregnancy selected{};
    const auto n=w.descendant_count+2;
    for(std::size_t i=0;i<n;++i) {
        const auto& a=active_at(w,i);
        if(!eligible_parent(a,w.tick)) continue;
        for(std::size_t j=i+1;j<n;++j) {
            const auto& b=active_at(w,j);
            if(!valid_pair(a.id,b.id)||!eligible_parent(b,w.tick)||
               std::abs(int(a.x)-int(b.x))+std::abs(int(a.y)-int(b.y))>2) continue;
            ++count;
            if(count==1||(w.streams[reproduction].bounded(count,roll)&&roll==0))
                selected={a.id,b.id,w.tick+gestation_ticks};
        }
    }
    if(count&&w.streams[reproduction].bounded(4,roll)&&roll==0) w.pregnancy=selected;
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
}
inline void update_lifecycle(World& w) noexcept {
    for(std::size_t i=0;i<w.descendant_count;++i) {
        auto& c=w.descendants[i];
        if(c.purpose==Purpose::none&&life_stage(c,w.tick)>=LifeStage::adult)
            c.purpose=choose_purpose(c);
    }
    if(w.pregnancy.due_tick) {
        // Mortality and compaction already ran, so resolve by ID, not pointer/slot.
        const auto* a=live_citizen(w,w.pregnancy.parent1);
        const auto* b=live_citizen(w,w.pregnancy.parent2);
        if(!a||!b) {
            if(w.lost_gestations<std::numeric_limits<std::uint64_t>::max()) ++w.lost_gestations;
            w.pregnancy={};
        } else if(w.tick==w.pregnancy.due_tick) {
            deliver(w,*a,*b);
            w.pregnancy={};
        }
        return;
    }
    attempt_conception(w);
}
} // namespace genesis
