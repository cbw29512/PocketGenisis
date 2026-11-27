#pragma once
#include "survival.hpp"
#include "lifecycle.hpp"
#include "mortality.hpp"

namespace genesis {
struct Production {
    std::uint64_t food{}, water{}, wood{};
};
// Natural cap only limits new production; it does not destroy existing stock.
inline unsigned renew_stock(std::uint16_t& stock, unsigned amount) noexcept {
    constexpr unsigned cap=100;
    if(stock>=cap) return 0;
    const auto added=std::min(amount,cap-stock);
    stock=static_cast<std::uint16_t>(stock+added);
    return added;
}
// Optional observation only: phase boundaries are not saved state or rules.
enum class TickPhase : std::size_t { survival, disease, mortality, lifecycle, renewal, count };
// One authoritative tick; natural renewal happens after citizen consumption.
namespace detail {
template<class Validate,class Observe>
inline bool step_world_observed(World& w,Production& produced,const Validate& validate,
                                const Observe& observe) noexcept {
    produced={};
    if(!step_survival_checked(w,validate)) return false;
    observe(TickPhase::survival);
    advance_diseases(w);
    observe(TickPhase::disease);
    update_mortality(w);
    observe(TickPhase::mortality);
    update_lifecycle(w);
    observe(TickPhase::lifecycle);
    if(w.tick%60!=0) {observe(TickPhase::renewal);return true;}
    constexpr unsigned food[]={2,2,1,0};
    constexpr unsigned water[]={3,1,2,2};
    constexpr unsigned wood[]={1,1,0,0};
    const auto s=season(w);
    for(auto& t:w.tiles) {
        produced.food+=renew_stock(t.food,food[s]);
        produced.water+=renew_stock(t.water,water[s]);
        produced.wood+=renew_stock(t.wood,wood[s]);
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
