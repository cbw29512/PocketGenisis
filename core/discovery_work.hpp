#pragma once
#include "ecology.hpp"
#include "discovery.hpp"

namespace genesis::discovery {
// Disposable policy: an eligible productive task takes priority over wandering.
// This leaves normal production step_world and its replay rules unchanged.
struct ReserveWork {
    const State& state;
    std::uint64_t cadence;
    bool operator()(const World& w,const Citizen& c) const noexcept {
        if(c.id<1||c.id>2||!available(c)||!cadence) return false;
        const unsigned owner=static_cast<unsigned>(c.id-1);
        const auto next=w.tick+1; // caller checked clock exhaustion
        if(state.job.active) return state.job.owner==owner;
        const auto& other=w.founders[1-owner];
        // Both participants reserve discretionary time for a local lesson.
        // Do not force reunion or interrupt survival.
        if(next-state.last_teaching>=60&&other.health>=50&&
           other.hunger<12&&other.thirst<12&&distance_between(c,other)<=2) {
            for(unsigned i=0;i<2;++i) for(unsigned k=0;k<tech_count;++k) {
                const auto tech=static_cast<Tech>(k);
                if((state.known[1-i]&tech_bit(tech))&&eligible(state.known[i],tech)) return true;
            }
        }
        if(next-state.last_opportunity<cadence) return false;
        const auto& tile=w.tiles[std::size_t(c.y)*width+c.x];
        for(unsigned k=0;k<tech_count;++k)
            if(eligible(state.known[owner],static_cast<Tech>(k))&&affordable(tile,rules[k])) return true;
        return false;
    }
};
inline bool step_with_research(World& w,State& s,Production& p,Event& event,
                               std::uint64_t cadence=normal_cadence) noexcept {
    event={};
    if(!detail::step_world_observed(w,p,valid_population,[](TickPhase) noexcept {},
                                   ReserveWork{s,cadence})) return false;
    event=advance(w,s,cadence);
    return true;
}
}
