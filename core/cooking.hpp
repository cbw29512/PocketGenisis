#pragma once
#include "food_store.hpp"

namespace genesis::cooking {
inline constexpr unsigned bonus=6;
struct State {std::uint64_t meals{},wood{},relief{};};
inline bool valid(const State& s) noexcept {
    return s.meals==s.wood&&s.relief>=s.meals&&s.relief<=s.meals*bonus;
}
struct MealPolicy {
    const discovery::State& research;
    const construction::State& sites;
    storage::State& reserve;
    State& cooked;
    bool operator()(World& w,Citizen& recipient,const Citizen& provider) const noexcept {
        return storage::ReserveFood{sites,reserve}(w,recipient,provider);
    }
    void after_food(World& w,Citizen& recipient,const Citizen& provider) const noexcept {
        const auto& fire=sites.sites[construction::campfire];
        // Knowledge belongs to the cook, not globally to the world.
        if(provider.id<1||provider.id>2||provider.health<50||
           !(research.known[provider.id-1]&discovery::tech_bit(discovery::cooking))||
           fire.stage!=construction::Stage::complete||fire.tile>=w.tiles.size()||
           recipient.action!=Action::eat||!recipient.hunger||
           distance_between(recipient,provider)>1) return;
        const int distance=std::abs(int(provider.x)-int(fire.tile%width))+
                           std::abs(int(provider.y)-int(fire.tile/width));
        if(distance>1) return;
        auto& fuel=w.tiles[fire.tile].wood;
        if(!fuel) return;
        // Actual fuel expenditure, bounded additional nutrition; no new food.
        const auto relief=std::min<unsigned>(bonus,recipient.hunger);
        --fuel;recipient.hunger=static_cast<std::uint8_t>(recipient.hunger-relief);
        ++cooked.meals;++cooked.wood;cooked.relief+=relief;
    }
};
inline bool step(World& w,discovery::State& research,construction::State& sites,
                 storage::State& reserve,State& cooked,Production& p,
                 discovery::Event& re,construction::Event& ce,std::uint64_t cadence) noexcept {
    return storage::step_with_food(w,research,sites,reserve,p,re,ce,cadence,
                                    MealPolicy{research,sites,reserve,cooked});
}
}
