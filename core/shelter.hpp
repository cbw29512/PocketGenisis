#pragma once
#include "cooking.hpp"

namespace genesis::shelter {
inline constexpr unsigned recovery_slots=2;
struct State {std::uint64_t extra_recovery{};};
struct Recovery {
    const construction::State& sites;
    State& state;
    // Fresh policy per authoritative tick: no persistent slot reservation.
    mutable unsigned used{};
    void operator()(Citizen& c) const noexcept {
        const auto& site=sites.sites[construction::hut];
        if(used>=recovery_slots||site.stage!=construction::Stage::complete||
           c.action!=Action::idle||!c.health||c.health>=100||
           construction::location(c)!=site.tile) return;
        // Called only after the ordinary engine has confirmed natural healing.
        ++c.health;++used;++state.extra_recovery;
    }
};
inline bool step(World& w,discovery::State& research,construction::State& sites,
                 storage::State& reserve,cooking::State& cooked,State& shelter,Production& p,
                 discovery::Event& re,construction::Event& ce,std::uint64_t cadence) noexcept {
    return storage::step_with_food(w,research,sites,reserve,p,re,ce,cadence,
        cooking::MealPolicy{research,sites,reserve,cooked},Recovery{sites,shelter});
}
}
