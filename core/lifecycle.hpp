#pragma once
#include "world.hpp"
#include <algorithm>
#include <cstdlib>

namespace genesis {
// Called only after validated survival tick; capacity is reserved at conception.
inline void update_lifecycle(World& w) noexcept {
    for(std::size_t i=0;i<w.descendant_count;++i) {
        auto& c=w.descendants[i];
        if(c.purpose==Purpose::none && life_stage(c,w.tick)>=LifeStage::adult)
            c.purpose=choose_purpose(c);
    }
    if(w.founder_due_tick) {
        if(w.tick!=w.founder_due_tick) return;
        auto& child=w.descendants[w.descendant_count++];
        child=Citizen{};
        child.id=w.next_citizen_id++;
        child.parent1=1; child.parent2=2;
        child.birth_tick=w.tick;
        child.x=w.founders[0].x; child.y=w.founders[0].y;
        for(std::size_t i=0;i<child.personality.size();++i) {
            std::uint32_t roll{};
            w.streams[reproduction].bounded(21,roll);
            const int mean=(w.founders[0].personality[i]+w.founders[1].personality[i])/2;
            child.personality[i]=static_cast<std::uint8_t>(std::clamp(mean+static_cast<int>(roll)-10,0,100));
        }
        std::uint32_t lifespan_roll{};
        w.streams[reproduction].bounded(21,lifespan_roll);
        child.lifespan_years=static_cast<std::uint8_t>(60+lifespan_roll);
        w.founder_due_tick=0;
        return; // Do not conceive again on the same tick.
    }
    if(w.tick%600 || w.descendant_count>=descendant_capacity ||
       w.descendant_count+w.archive_count>=history_capacity ||
       w.tick>std::numeric_limits<std::uint64_t>::max()-gestation_ticks) return;
    const auto& a=w.founders[0]; const auto& b=w.founders[1];
    if(a.health<50||b.health<50||a.hunger>24||a.thirst>24||b.hunger>24||b.thirst>24 ||
       std::abs(int(a.x)-int(b.x))+std::abs(int(a.y)-int(b.y))>2) return;
    std::uint32_t roll{};
    w.streams[reproduction].bounded(4,roll);
    if(!roll) w.founder_due_tick=w.tick+gestation_ticks;
}
} // namespace genesis
