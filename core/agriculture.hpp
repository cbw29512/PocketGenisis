#pragma once
#ifndef PG_AGRICULTURE_PROBE
#error "Agriculture is an isolated probe profile"
#endif
#include "settlement.hpp"

namespace genesis::agriculture {
inline constexpr unsigned seed_cost=2,harvest_yield=8,stock_cap=100;
inline constexpr std::uint64_t work_interval=120;
inline constexpr int tending_radius=3;
struct State {
    std::uint8_t stage{};
    std::uint64_t last_work{},plants{},tended{},water{},harvests{},food{};
};
inline bool valid(const State& s) noexcept {
    return s.stage<=4&&s.plants==s.harvests+(s.stage!=0)&&
        s.water==s.harvests*4+s.stage&&s.tended==s.harvests*3+(s.stage?s.stage-1:0)&&
        s.food==s.harvests*harvest_yield;
}
enum class Task : std::uint8_t { none,plant,tend,harvest };
inline Task task(const World& w,const construction::State& sites,const State& s,
                 std::uint64_t tick) noexcept {
    const auto& plot=sites.sites[construction::field];
    if(plot.stage!=construction::Stage::complete||plot.tile>=w.tiles.size()||s.stage>4) return Task::none;
    const auto& stock=w.tiles[plot.tile];
    const bool winter=(tick%ticks_per_year)/(ticks_per_year/4)==3;
    if(!s.stage)
        return !winter&&stock.food>=12+seed_cost&&stock.food<=stock_cap-harvest_yield&&stock.water?
            Task::plant:Task::none;
    if(tick<s.last_work||tick-s.last_work<work_interval) return Task::none;
    if(s.stage==4) return stock.food<=stock_cap-harvest_yield?Task::harvest:Task::none;
    return !winter&&stock.water?Task::tend:Task::none;
}
inline bool farmer(const discovery::State& knowledge,const Citizen& c) noexcept {
    return is_founder(c)&&discovery::available(c)&&
           (knowledge.known[c.id-1]&discovery::tech_bit(discovery::farming));
}
inline Task work(World& w,const discovery::State& knowledge,const construction::State& sites,
                 State& s,Citizen& c) noexcept {
    if(!farmer(knowledge,c)||construction::location(c)!=sites.sites[construction::field].tile)
        return Task::none;
    const auto action=task(w,sites,s,w.tick);
    if(action==Task::none) return action;
    auto& stock=w.tiles[sites.sites[construction::field].tile];
    if(action==Task::plant) {
        stock.food-=seed_cost;--stock.water;s.stage=1;++s.plants;++s.water;
    } else if(action==Task::tend) {
        --stock.water;++s.stage;++s.tended;++s.water;
    } else {
        stock.food+=harvest_yield;s.stage=0;++s.harvests;s.food+=harvest_yield;
    }
    s.last_work=w.tick;return action;
}
// A prepared field replaces wild food growth on exactly its own tile.
struct Renewal {
    const construction::State& sites;
    bool operator()(std::size_t tile) const noexcept {
        const auto& plot=sites.sites[construction::field];
        return plot.stage!=construction::Stage::complete||plot.tile!=tile;
    }
};
inline bool viable_tending(const World& w,const construction::State& sites,const State& farm,
                           std::uint64_t tick) noexcept {
    const auto& plot=sites.sites[construction::field];
    if(!farm.stage||farm.stage>4||plot.stage!=construction::Stage::complete||plot.tile>=w.tiles.size()) return false;
    const auto& stock=w.tiles[plot.tile];
    if(farm.stage==4) return stock.food<=stock_cap-harvest_yield;
    return (tick%ticks_per_year)/(ticks_per_year/4)!=3&&stock.food>=12&&stock.water>=12;
}
struct Work {
    const discovery::State& research;
    const construction::State& sites;
    const storage::State& reserve;
    const State& farm;
    settlement::State& home;
    std::uint64_t cadence;
    mutable std::uint64_t chosen{};
    bool operator()(const World& w,Citizen& c) const noexcept {
        if(storage::ReserveWork{research,sites,reserve,cadence}(w,c)) return true;
        if(research.job.active||construction::active(sites)>=0) return false;
        if(!chosen&&farmer(research,c)&&task(w,sites,farm,w.tick+1)!=Task::none) {
            const auto& plot=sites.sites[construction::field];
            const int x=plot.tile%width,y=plot.tile/width;
            const int distance=std::abs(int(c.x)-x)+std::abs(int(c.y)-y);
            if(!distance) {chosen=c.id;return true;}
            if(distance<=settlement::home_radius&&c.hunger<=8&&c.thirst<=8) {
                move_toward(c,x,y);return true;
            }
        }
        // During a viable unfinished crop, nearby farmers tend to stay in
        // its vicinity between work opportunities, but never override needs.
        if(farmer(research,c)&&c.hunger<=8&&c.thirst<=8&&
           viable_tending(w,sites,farm,w.tick+1)) {
            const auto tile=sites.sites[construction::field].tile;
            const int x=tile%width,y=tile/width;
            const int distance=std::abs(int(c.x)-x)+std::abs(int(c.y)-y);
            if(distance>=tending_radius&&distance<=settlement::home_radius) {
                move_toward(c,x,y);return true;
            }
        }
        return settlement::return_home(w,sites,c,home);
    }
};
inline bool step(World& w,discovery::State& research,construction::State& sites,
                 storage::State& reserve,cooking::State& cooked,shelter::State& sheltered,
                 settlement::State& home,State& farm,Production& p,
                 discovery::Event& re,construction::Event& ce,std::uint64_t cadence) noexcept {
    const bool busy=research.job.active||construction::active(sites)>=0;
    const auto deposits=reserve.deposited;
    const Work policy{research,sites,reserve,farm,home,cadence};
    if(!storage::step_with_policies(w,research,sites,reserve,p,re,ce,cadence,
        cooking::MealPolicy{research,sites,reserve,cooked},shelter::Recovery{sites,sheltered},policy,Renewal{sites})) return false;
    if(policy.chosen&&!busy&&!research.job.active&&construction::active(sites)<0&&
        re.kind==discovery::EventKind::none&&ce.kind==construction::EventKind::none&&deposits==reserve.deposited) {
        if(work(w,research,sites,farm,w.founders[policy.chosen-1])==Task::harvest) p.food+=harvest_yield;
    }
    return true;
}
}
