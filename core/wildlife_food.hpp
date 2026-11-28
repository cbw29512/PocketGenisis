#pragma once
#include "wildlife.hpp"
#include "cooking.hpp"
#include "freshwater.hpp"

namespace genesis::wildlife {
// Candidate food policy, no new individual state or RNG. Existing meat
// is usable at any local site. Take prey only when plants are scarce.
inline constexpr unsigned hunting_food_threshold=24;
struct MealPolicy {
    cooking::MealPolicy cooked;
    Wildlife& wild;
    Totals& totals;
    freshwater::State* fish=nullptr;
    bool operator()(World& w,Citizen& recipient,const Citizen& provider) const noexcept {
        return cooked(w,recipient,provider);
    }
    void after_food(World& w,Citizen& recipient,const Citizen& provider) const noexcept {
        cooked.after_food(w,recipient,provider);
    }
};
inline bool hunter(const Citizen& c,std::uint64_t tick) noexcept {
    return c.health>=50&&c.action==Action::idle&&life_stage(c,tick)==LifeStage::adult;
}
inline bool prey_available(const WildlifeTile& t) noexcept {
    for(std::size_t k=0;k<wolf;++k) if(t.count[k]) return true;
    return false;
}
// ADL extension for care.hpp. Stored meat can feed a dependent, but
// only an independently hungry eligible adult can take wild prey.
inline bool preferred_food(World& w,Citizen& recipient,const Citizen& provider,
                           const MealPolicy& policy) noexcept {
    if(recipient.hunger<12||recipient.action!=Action::idle||
       !provider.health||distance_between(recipient,provider)>1) return false;
    const auto i=std::size_t(provider.y)*width+provider.x;
    if(i>=w.tiles.size()) return false;
    const bool allowed=recipient.id==provider.id&&hunter(provider,w.tick)&&
                       !policy.wild.developed[i]&&w.tiles[i].food<=hunting_food_threshold;
    // Existing meat first; fish before taking a new terrestrial animal.
    if(take_meat(policy.wild.tiles[i],policy.totals,false)) {
        satisfy_need(recipient,false);return true;
    }
    if(policy.fish&&recipient.id==provider.id&&hunter(provider,w.tick)&&
       w.tiles[i].food<=hunting_food_threshold&&freshwater::catch_one(*policy.fish,i)) {
        satisfy_need(recipient,false);return true;
    }
    if(!take_meat(policy.wild.tiles[i],policy.totals,allowed)) return false;
    satisfy_need(recipient,false);return true;
}
// ADL extension for survival.hpp. Search only the original Manhattan
// neighborhood; movement and consumption are distinct actual actions.
inline ResourceTarget seek_food(const World& w,const Citizen& c,const MealPolicy& policy) noexcept {
    return find_local_resource(w,c.x,c.y,[&](std::size_t i,const Tile& t) noexcept {
        return t.food>0||policy.wild.tiles[i].meat>0||
               (hunter(c,w.tick)&&((policy.fish&&freshwater::available(*policy.fish,i))||
                                   (!policy.wild.developed[i]&&prey_available(policy.wild.tiles[i]))));
    });
}
}
