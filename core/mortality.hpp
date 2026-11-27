#pragma once
#include "disease.hpp"

namespace genesis {
inline void update_health(Citizen& c,bool immortal) noexcept {
    if(c.hunger==100||c.thirst==100) {
        if(c.health>(immortal?1:0)) --c.health;
    } else if(c.hunger<=24&&c.thirst<=24&&c.health<100&&!is_ill(c)) {
        ++c.health;
    }
}
// Admission reserves archive capacity. No allocation or overwrite on death.
inline void archive_death(World& w,const Citizen& c,DeathCause cause) noexcept {
    w.archive[w.archive_count++]={c,w.tick,cause};
}
inline void update_mortality(World& w) noexcept {
    for(auto& f:w.founders) update_health(f,true);
    std::size_t write=0;
    const auto old_count=w.descendant_count;
    for(std::size_t i=0;i<old_count;++i) {
        auto& c=w.descendants[i];
        const bool disease_death=c.health==0;
        if(!disease_death) update_health(c,false);
        const bool expired=(w.tick-c.birth_tick)/ticks_per_year>=c.lifespan_years;
        if(c.health==0||expired) {
            const auto cause=c.health!=0?DeathCause::old_age:
                disease_death?DeathCause::disease:
                c.thirst==100?DeathCause::dehydration:DeathCause::starvation;
            archive_death(w,c,cause);
        } else {
            if(write!=i) w.descendants[write]=c;
            ++write;
        }
    }
    for(std::size_t i=write;i<old_count;++i) w.descendants[i]=Citizen{};
    w.descendant_count=write;
}
} // namespace genesis
