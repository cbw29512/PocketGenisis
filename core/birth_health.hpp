#pragma once
#include "mortality.hpp"
#include <algorithm>

namespace genesis {
// Simulation-only basis points; calibrate over many seeds before release.
inline unsigned birth_risk(const Citizen& a,const Citizen& b) noexcept {
    const int health=std::min(a.health,b.health);
    const int need=std::max({a.hunger,a.thirst,b.hunger,b.thirst});
    return static_cast<unsigned>(std::min(1500,100+10*(100-health)+5*std::max(0,need-24)));
}
inline void apply_birth_health(World& w,Citizen& child,const Citizen& a,const Citizen& b) noexcept {
    const auto risk=birth_risk(a,b);
    std::uint32_t roll{};
    // A separate stream prevents health draws from perturbing inherited traits.
    w.streams[health_events].bounded(10000,roll);
    if(roll<risk/5) {
        child.birth_outcome=BirthOutcome::fatal;
        child.health=0;
    } else if(roll<risk) {
        child.birth_outcome=BirthOutcome::complicated;
        child.health=35;
    }
}
} // namespace genesis
