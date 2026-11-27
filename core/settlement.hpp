#pragma once
#include "shelter.hpp"

namespace genesis::settlement {
inline constexpr unsigned home_stock_min=12,free_need_max=8,return_period=120,return_window=12;
inline constexpr int home_radius=8;
struct State {std::uint64_t steps{},arrivals{};};
inline bool valid(const State& state) noexcept {return state.arrivals<=state.steps;}
inline bool return_home(World const& w,const construction::State& sites,Citizen& c,State& state) noexcept {
    const auto& home=sites.sites[construction::hut];
    if(!is_founder(c)||home.stage!=construction::Stage::complete||
       home.tile>=w.tiles.size()||founder_incapacitated(c)||
       c.action!=Action::idle||c.hunger>free_need_max||c.thirst>free_need_max) return false;
    if(c.health>=90&&w.tick%return_period>=return_window) return false;
    const auto& stock=w.tiles[home.tile];
    if(stock.food<home_stock_min||stock.water<home_stock_min) return false;
    const int x=home.tile%width,y=home.tile/width;
    const int distance=std::abs(int(c.x)-x)+std::abs(int(c.y)-y);
    if(!distance||distance>home_radius) return false;
    // Use exactly one ordinary movement action; never teleport or trap citizens.
    move_toward(c,x,y);++state.steps;
    if(construction::location(c)==home.tile) ++state.arrivals;
    return true;
}
struct Work {
    const discovery::State& research;
    const construction::State& sites;
    const storage::State& reserve;
    State& state;
    std::uint64_t cadence;
    bool operator()(const World& w,Citizen& c) const noexcept {
        if(storage::ReserveWork{research,sites,reserve,cadence}(w,c)) return true;
        if(research.job.active||construction::active(sites)>=0) return false;
        return return_home(w,sites,c,state);
    }
};
inline bool step(World& w,discovery::State& research,construction::State& sites,
                 storage::State& reserve,cooking::State& cooked,shelter::State& sheltered,
                 State& state,Production& p,discovery::Event& re,construction::Event& ce,
                 std::uint64_t cadence) noexcept {
    return storage::step_with_policies(w,research,sites,reserve,p,re,ce,cadence,
        cooking::MealPolicy{research,sites,reserve,cooked},shelter::Recovery{sites,sheltered},
        Work{research,sites,reserve,state,cadence});
}
}
