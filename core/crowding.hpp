#pragma once
#include <algorithm>
#include <cstdint>

namespace genesis {
// Nearby people, not total world population. No physical room scale yet.
inline unsigned crowding_odds_percent(unsigned neighbors) noexcept {
    return 100u+25u*(std::clamp(neighbors,2u,6u)-2u);
}
// Convert an odds multiplier to a bounded probability, not a risk multiplier.
inline unsigned crowded_exposure(unsigned basis_points,unsigned neighbors) noexcept {
    const auto p=std::min(10000u,basis_points);
    const auto odds=crowding_odds_percent(neighbors);
    return static_cast<unsigned>(std::uint64_t(p)*odds*10000u/
        (std::uint64_t(10000u-p)*100u+std::uint64_t(p)*odds));
}
// Acute needs are a provisional stress proxy, not measured clinical wasting.
inline unsigned stressed_disease_damage(unsigned damage,unsigned hunger,unsigned thirst) noexcept {
    const auto stress=std::clamp(std::max(hunger,thirst),60u,100u)-60u;
    return damage+(damage*stress)/40u;
}
} // namespace genesis
