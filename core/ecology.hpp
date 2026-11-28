#pragma once
#include "survival.hpp"
#include "lifecycle.hpp"
#include "mortality.hpp"

namespace genesis {
struct Production {
    std::uint64_t food{}, water{}, wood{};
};
// Natural cap only limits new production; it does not destroy existing stock.
inline unsigned renew_stock(std::uint16_t& stock,unsigned amount,unsigned cap=100) noexcept {
    if(stock>=cap) return 0;
    const auto added=std::min(amount,cap-stock);
    stock=static_cast<std::uint16_t>(stock+added);
    return added;
}
// Cultivated land may suppress wild food renewal without changing water/wood.
struct NaturalFoodRenewal {
    constexpr bool operator()(std::size_t) const noexcept {return true;}
};
// Default renewal transaction preserves the previous uniform rules.
// Alternate policy overloads may specialize this through ADL.
template<class Renewal>
inline void renew_tile(Tile& t,Production& p,std::size_t i,std::uint64_t,
                       unsigned food,unsigned water,unsigned wood,const Renewal& renewal) noexcept {
    if(renewal(i)) p.food+=renew_stock(t.food,food);
    p.water+=renew_stock(t.water,water);
    p.wood+=renew_stock(t.wood,wood);
}
// Optional observation only: phase boundaries are not saved state or rules.
enum class TickPhase : std::size_t { survival, disease, mortality, lifecycle, renewal, count };
// One authoritative tick; natural renewal happens after citizen consumption.
namespace detail {
template<class Validate,class Observe,class Work=NoDiscretionaryWork,class Food=NoReserveFood,class Recovery=NoRecoveryBonus,class Renewal=NaturalFoodRenewal>
inline bool step_world_observed(World& w,Production& produced,const Validate& validate,
                                const Observe& observe,const Work& work={},const Food& reserve_food={},const Recovery& recovery={},const Renewal& renewal={}) noexcept {
    produced={};
    if(!step_survival_checked(w,validate,work,reserve_food)) return false;
    observe(TickPhase::survival);
    advance_diseases(w);
    observe(TickPhase::disease);
    update_mortality(w,recovery);
    observe(TickPhase::mortality);
    update_lifecycle(w);
    observe(TickPhase::lifecycle);
    if(w.tick%60!=0) {observe(TickPhase::renewal);return true;}
    constexpr unsigned food[]={2,2,1,0};
    constexpr unsigned water[]={3,1,2,2};
    constexpr unsigned wood[]={1,1,0,0};
    const auto s=season(w);
    for(std::size_t i=0;i<w.tiles.size();++i) {
        renew_tile(w.tiles[i],produced,i,w.tick,food[s],water[s],wood[s],renewal);
    }
    observe(TickPhase::renewal);
    return true;
}
template<class Validate>
inline bool step_world_checked(World& w,Production& produced,const Validate& validate) noexcept {
    return step_world_observed(w,produced,validate,[](TickPhase) noexcept {});
}
} // namespace detail
inline bool step_world(World& w,Production& produced) noexcept {
    return detail::step_world_checked(w,produced,valid_population);
}
} // namespace genesis
