#pragma once
#include "founder_recovery.hpp"
namespace genesis::fatigue {
// Optional disposable profile. Two bytes of logical state per citizen.
// Stagger accumulation by stable identity; no RNG or spatial scan.
inline constexpr unsigned limit=100,sleep_at=72,wake_at=24,recover_per_tick=4;
inline constexpr std::uint64_t awake_interval=8;
inline void accrue(Citizen& c,std::uint64_t tick) noexcept {
    if((tick%awake_interval+c.id%awake_interval)%awake_interval==0&&c.fatigue<limit)
        ++c.fatigue;
    if(c.fatigue>=sleep_at) c.rest_intent=1;
}
inline bool rest(Citizen& c) noexcept {
    if(!c.rest_intent) return false;
    // A sleep episode can be interrupted by food, water or care. The
    // intent persists until recovery actually reaches the lower bound.
    c.action=Action::rest;
    c.fatigue=static_cast<std::uint8_t>(c.fatigue>recover_per_tick?
                                      c.fatigue-recover_per_tick:0);
    if(c.fatigue<=wake_at) c.rest_intent=0;
    return true;
}
inline bool partner_needs_help(const World& w,const Citizen& c) noexcept {
    if(!is_founder(c)||c.health<50) return false;
    const auto& other=w.founders[c.id==1?1:0];
    return founder_incapacitated(other)&&urgent_need(other);
}
}
