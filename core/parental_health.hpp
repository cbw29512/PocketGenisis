#pragma once
#include "birth_health.hpp"

namespace genesis {
// Provisional fictional risk per delivery, not human medical statistics.
// Reuse the already defined parental-health and resource-stress proxy.
inline unsigned parental_risk(const Citizen& a,const Citizen& b) noexcept {
    return birth_risk(a,b)/2;
}
inline void apply_parental_health(World& w,Citizen& a,Citizen& b) noexcept {
    const auto risk=parental_risk(a,b);
    std::uint32_t roll{};
    w.streams[parental_health_events].bounded(10000,roll);
    if(roll>=risk) return;
    std::uint32_t choice{};w.streams[parental_health_events].bounded(2,choice);
    auto& affected=choice?b:a;
    // One parent is affected. Severe complications can kill mortals;
    // the existing immortal health floor applies to both founders.
    const int damage=roll<risk/10?100:30;
    const int floor=affected.id<=2?1:0;
    affected.health=static_cast<std::uint8_t>(std::max(floor,int(affected.health)-damage));
}
// Run after ALL due births, while the live array still has stable slots.
// No new pregnancy can reference a fatally injured parent in this phase.
inline void retire_parental_deaths(World& w) noexcept {
    std::size_t write=0;
    const auto n=w.descendant_count;
    for(std::size_t i=0;i<n;++i) {
        const auto& c=w.descendants[i];
        if(!c.health) archive_death(w,c,DeathCause::parental_childbirth);
        else {
            if(write!=i) w.descendants[write]=c;
            ++write;
        }
    }
    for(std::size_t i=write;i<n;++i) w.descendants[i]=Citizen{};
    w.descendant_count=write;
    // Deaths before delivery and fatal newborns can have lower/higher IDs.
    // Sort only the current tick's suffix; never disturb committed history.
    auto first=w.archive_count;
    while(first&&w.archive[first-1].death_tick==w.tick) --first;
    std::sort(w.archive.begin()+first,w.archive.begin()+w.archive_count,
        [](const DeathRecord& a,const DeathRecord& b){return a.citizen.id<b.citizen.id;});
}
} // namespace genesis
