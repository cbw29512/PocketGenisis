#pragma once
#include "world.hpp"
#include <algorithm>
#include <cstdlib>

namespace genesis::discovery {
enum Tech : std::uint8_t { fire,stone_tools,shelter,cooking,food_storage,
#ifdef PG_AGRICULTURE_PROBE
    farming,
#endif
    tech_count };
using Mask=std::uint16_t;
inline constexpr Mask tech_bit(Tech t) noexcept {return static_cast<Mask>(1u<<t);}
struct Rule {
    const char* name;
    Mask prerequisites;
    unsigned wood,stone,food,chance;
};
inline constexpr std::array<Rule,tech_count> rules{{
    {"Fire",0,2,1,0,70},
    {"Stone tools",0,0,3,0,70},
    {"Basic shelter",tech_bit(stone_tools),4,2,0,50},
    {"Cooking",tech_bit(fire),1,0,2,50},
    {"Food storage",tech_bit(stone_tools),3,1,2,50}
#ifdef PG_AGRICULTURE_PROBE
    ,{"Farming",static_cast<Mask>(tech_bit(stone_tools)|tech_bit(food_storage)),2,1,4,35}
#endif
}};
inline constexpr unsigned work_required=30;
inline constexpr std::uint64_t normal_cadence=43200,probe_cadence=60,stall_limit=600;
struct Record {std::uint64_t tick{},founder{};};
struct Experiment {
    std::uint8_t owner{},tech{},work{};
    bool active{};
    std::uint64_t last_work_tick{};
};
struct State {
    std::array<Mask,2> known{};
    std::array<Record,tech_count> first{};
    Experiment job{};
    std::uint64_t attempts{},successes{},failures{},abandoned{},last_opportunity{},last_teaching{};
};
enum class EventKind : std::uint8_t { none,started,failed,discovered,learned,taught,abandoned };
struct Event {EventKind kind{};std::uint8_t tech{};std::uint64_t founder{};};
inline bool available(const Citizen& c) noexcept {
    return c.health>=50&&c.hunger<12&&c.thirst<12&&c.action==Action::idle;
}
inline bool eligible(Mask known,Tech tech) noexcept {
    const auto& rule=rules[tech];
    return !(known&tech_bit(tech))&&(known&rule.prerequisites)==rule.prerequisites;
}
inline bool affordable(const Tile& tile,const Rule& rule) noexcept {
    return tile.wood>=rule.wood&&tile.stone>=rule.stone&&tile.food>=rule.food;
}
inline unsigned chance(const Citizen& c,Tech tech) noexcept {
    // Centered traits add at most five percentage points either way.
    const int modifier=(int(c.personality[0])+int(c.personality[3])-100)/20;
    return static_cast<unsigned>(int(rules[tech].chance)+modifier);
}
inline Tile& local(World& w,const Citizen& c) noexcept {
    return w.tiles[std::size_t(c.y)*width+c.x];
}
inline Event teach(const World& w,State& s) noexcept {
    // A teaching opportunity waits for free time instead of sampling one fixed phase.
    if(s.job.active||w.tick-s.last_teaching<60||!available(w.founders[0])||!available(w.founders[1])) return {};
    const auto& a=w.founders[0];const auto& b=w.founders[1];
    if(std::abs(int(a.x)-int(b.x))+std::abs(int(a.y)-int(b.y))>2) return {};
    for(unsigned i=0;i<2;++i) for(unsigned k=0;k<tech_count;++k) {
        const auto tech=static_cast<Tech>(k);
        if(s.job.active&&s.job.owner==i&&s.job.tech==k) continue;
        if((s.known[1-i]&tech_bit(tech))&&eligible(s.known[i],tech)) {
            s.known[i]|=tech_bit(tech);s.last_teaching=w.tick;
            return {EventKind::taught,static_cast<std::uint8_t>(k),w.founders[i].id};
        }
    }
    return {};
}
// Called once after a successful authoritative tick. The disposable policy may
// reserve discretionary time; urgent survival actions are never replaced.
inline Event advance(World& w,State& s,std::uint64_t cadence=normal_cadence) noexcept {
    if(!cadence||!w.tick) return {};
    if(s.job.active) {
        auto& job=s.job;auto& c=w.founders[job.owner];
        if(available(c)) {
            job.last_work_tick=w.tick;
            ++job.work;
        } else if(w.tick-job.last_work_tick>=stall_limit) {
            job.active=false;++s.abandoned;
            return {EventKind::abandoned,job.tech,c.id};
        }
        if(job.work==work_required) {
            const auto tech=static_cast<Tech>(job.tech);
            std::uint32_t roll{};w.streams[research].bounded(100,roll);
            job.active=false;
            if(roll>=chance(c,tech)) {
                ++s.failures;return {EventKind::failed,job.tech,c.id};
            }
            ++s.successes;s.known[job.owner]|=tech_bit(tech);
            const bool first=!s.first[tech].founder;
            if(first) s.first[tech]={w.tick,c.id};
            return {first?EventKind::discovered:EventKind::learned,job.tech,c.id};
        }
        return {}; // Do not teach during an active experiment.
    }
    // Prefer sharing an existing solution when both founders have free time.
    const auto shared=teach(w,s);
    if(shared.kind!=EventKind::none) return shared;
    // Cooldown measured from admission, not a one-tick sampling window.
    if(w.tick-s.last_opportunity>=cadence) {
        std::array<std::uint8_t,2*tech_count> candidates{};
        unsigned count=0;
        for(unsigned i=0;i<2;++i) {
            auto& c=w.founders[i];
            if(!available(c)) continue;
            for(unsigned k=0;k<tech_count;++k)
                if(eligible(s.known[i],static_cast<Tech>(k))&&affordable(local(w,c),rules[k]))
                    candidates[count++]=static_cast<std::uint8_t>(i*tech_count+k);
        }
        if(count) {
            std::uint32_t pick{};w.streams[research].bounded(count,pick);
            const auto selected=candidates[pick];
            const auto owner=static_cast<std::uint8_t>(selected/tech_count);
            const auto tech=static_cast<std::uint8_t>(selected%tech_count);
            auto& c=w.founders[owner];auto& tile=local(w,c);const auto& rule=rules[tech];
            tile.wood-=rule.wood;tile.stone-=rule.stone;tile.food-=rule.food;
            s.job={owner,tech,0,true,w.tick};++s.attempts;s.last_opportunity=w.tick;
            return {EventKind::started,tech,c.id};
        }
    }
    return {};
}
inline unsigned discovered(const State& s) noexcept {
    unsigned n=0;for(const auto& record:s.first) if(record.founder) ++n;return n;
}
} // namespace genesis::discovery
