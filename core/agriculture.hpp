#pragma once
#ifndef PG_AGRICULTURE_PROBE
#error "Agriculture is an isolated probe profile"
#endif
#include "settlement.hpp"
#ifdef PG_LIVESTOCK_PROBE
#include "farm_distribution.hpp"
#endif
#ifdef PG_IRRIGATION_PROBE
#include "farm_planning.hpp"
#endif

namespace genesis::agriculture {
inline constexpr unsigned seed_cost=2,harvest_yield=8,stock_cap=100;
inline constexpr std::uint64_t work_interval=120;
inline constexpr int tending_radius=3;
struct State {
    std::uint8_t stage{};
    std::uint64_t last_work{},plants{},tended{},water{},harvests{},food{};
#ifdef PG_CROP_LOSS_PROBE
    std::uint64_t drought_losses{},spoilage_losses{},lost_water{},dry_since{};
#endif
#ifdef PG_LIVESTOCK_PROBE
    std::uint64_t distributed{}; // actual existing food moved; not harvested yield
#endif
};
inline bool valid(const State& s) noexcept {
#ifdef PG_CROP_LOSS_PROBE
    const auto losses=s.drought_losses+s.spoilage_losses;
    const auto lost=s.lost_water;
    if(lost<losses||lost>4*losses||
       (s.dry_since&&(!s.stage||s.stage>=4))) return false;
#else
    constexpr std::uint64_t losses=0,lost=0;
#endif
    return s.stage<=4&&s.plants==s.harvests+(s.stage!=0)+losses&&
        s.water==s.harvests*4+s.stage+lost&&s.tended==s.harvests*3+(s.stage?s.stage-1:0)+lost-losses&&
        s.food==s.harvests*harvest_yield;
}
enum class Task : std::uint8_t { none,plant,tend,harvest
#ifdef PG_LIVESTOCK_PROBE
    ,distribute
#endif
};
inline Task task(const World& w,const construction::State& sites,const State& s,
                 std::uint64_t tick,unsigned slot=construction::field) noexcept {
    if(slot>=construction::site_count||construction::kind_at(slot)!=construction::field) return Task::none;
    const auto& plot=sites.sites[slot];
    if(plot.stage!=construction::Stage::complete||plot.tile>=w.tiles.size()||s.stage>4) return Task::none;
    const auto& stock=w.tiles[plot.tile];
    const bool winter=(tick%ticks_per_year)/(ticks_per_year/4)==3;
#ifdef PG_LIVESTOCK_PROBE
    // A seed debit creates space rather than consuming storage capacity.
    // Planting at high stock is legitimate. Prefer distribution if a
    // real neighbor needs food, but do not require room for future output.
    if(((!s.stage&&!winter)||(s.stage==4&&tick>=s.last_work&&tick-s.last_work>=work_interval))&&
       stock.food>stock_cap-harvest_yield&&farm_distribution::destination(w,sites,plot.tile)>=0)
        return Task::distribute;
    if(!s.stage)
        return !winter&&stock.food>=12+seed_cost&&stock.water?Task::plant:Task::none;
#else
    if(!s.stage)
        return !winter&&stock.food>=12+seed_cost&&stock.food<=stock_cap-harvest_yield&&stock.water?
            Task::plant:Task::none;
#endif
    if(tick<s.last_work||tick-s.last_work<work_interval) return Task::none;
    if(s.stage==4) return stock.food<=stock_cap-harvest_yield?Task::harvest:Task::none;
    return !winter&&stock.water?Task::tend:Task::none;
}
inline bool farmer(const discovery::State& knowledge,const Citizen& c) noexcept {
    return is_founder(c)&&discovery::available(c)&&
           (knowledge.known[c.id-1]&discovery::tech_bit(discovery::farming));
}
inline Task work(World& w,const discovery::State& knowledge,const construction::State& sites,
                 State& s,Citizen& c,unsigned slot=construction::field) noexcept {
    if(slot>=construction::site_count||construction::kind_at(slot)!=construction::field||
       !farmer(knowledge,c)||construction::location(c)!=sites.sites[slot].tile)
        return Task::none;
    const auto action=task(w,sites,s,w.tick,slot);
    if(action==Task::none) return action;
    auto& stock=w.tiles[sites.sites[slot].tile];
#ifdef PG_LIVESTOCK_PROBE
    if(action==Task::distribute) {
        const auto moved=farm_distribution::deliver(w,sites,sites.sites[slot].tile);
        if(!moved) return Task::none;
        s.distributed+=moved;
        // Distribution does not water, grow, reset drought or advance the crop timer.
        return action;
    }
#endif
    if(action==Task::plant) {
        stock.food-=seed_cost;--stock.water;s.stage=1;++s.plants;++s.water;
    } else if(action==Task::tend) {
        --stock.water;++s.stage;++s.tended;++s.water;
    } else {
        stock.food+=harvest_yield;s.stage=0;++s.harvests;s.food+=harvest_yield;
    }
#ifdef PG_CROP_LOSS_PROBE
    // Successful work consumes actual water or harvests the crop; drought
    // tracking must not leak from a previous growth stage into a new crop.
    s.dry_since=0;
#endif
    s.last_work=w.tick;return action;
}
// A prepared field replaces wild food growth on exactly its own tile.
struct Renewal {
    const construction::State& sites;
    bool operator()(std::size_t tile) const noexcept {
        for(unsigned i=0;i<construction::field_count;++i) {
            const auto& plot=sites.sites[construction::field_slot(i)];
            if(plot.stage==construction::Stage::complete&&plot.tile==tile) return false;
        }
        return true;
    }
};
inline bool viable_tending(const World& w,const construction::State& sites,const State& farm,
                           std::uint64_t tick,unsigned slot=construction::field) noexcept {
    if(slot>=construction::site_count||construction::kind_at(slot)!=construction::field) return false;
    const auto& plot=sites.sites[slot];
    if(!farm.stage||farm.stage>4||plot.stage!=construction::Stage::complete||plot.tile>=w.tiles.size()) return false;
    const auto& stock=w.tiles[plot.tile];
    if(farm.stage==4) {
#ifdef PG_LIVESTOCK_PROBE
        // A full ripe plot remains a viable workplace when the farmer can
        // distribute surplus or consume its actual stock during normal needs.
        return stock.food>=12&&stock.water>=12;
#else
        return stock.food<=stock_cap-harvest_yield;
#endif
    }
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
        if(research.job.active||construction::active(sites)>=0) {
#ifdef PG_IRRIGATION_PROBE
            return farm_planning::seek(w,research,sites,c);
#else
            return false;
#endif
        }
#ifdef PG_IRRIGATION_PROBE
        if(farm_planning::seek(w,research,sites,c)) return true;
#endif
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
