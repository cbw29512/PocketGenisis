#pragma once
#ifndef PG_MULTI_FIELD_PROBE
#error "Multi-field farming requires its isolated profile"
#endif
#include "irrigation.hpp"

namespace genesis::multi_field {
struct State {
    std::array<agriculture::State,construction::field_count> crops{};
};
inline bool valid(const State& s) noexcept {
    for(const auto& c:s.crops) if(!agriculture::valid(c)) return false;
    return true;
}
inline std::uint64_t harvests(const State& s) noexcept {
    std::uint64_t n=0;for(const auto& c:s.crops) n+=c.harvests;return n;
}
inline unsigned completed(const construction::State& sites) noexcept {
    unsigned n=0;
    for(unsigned i=0;i<construction::field_count;++i)
        n+=sites.sites[construction::field_slot(i)].stage==construction::Stage::complete;
    return n;
}
struct Work {
    const discovery::State& research;
    const construction::State& sites;
    const storage::State& reserve;
    const State& farms;
    settlement::State& home;
    std::uint64_t cadence;
    mutable std::uint64_t chosen{};
    mutable unsigned selected{};
    bool operator()(const World& w,Citizen& c) const noexcept {
        if(storage::ReserveWork{research,sites,reserve,cadence}(w,c)) return true;
        if(research.job.active||construction::active(sites)>=0) return false;
        if(!chosen&&agriculture::farmer(research,c)) {
            int best=settlement::home_radius+1,ready=-1;
            for(unsigned i=0;i<construction::field_count;++i) {
                const auto slot=construction::field_slot(i);
                if(agriculture::task(w,sites,farms.crops[i],w.tick+1,slot)==agriculture::Task::none) continue;
                const auto tile=sites.sites[slot].tile;
                const int d=std::abs(int(c.x)-int(tile%width))+std::abs(int(c.y)-int(tile/width));
                if(d<best) {best=d;ready=int(i);}
            }
            if(ready>=0) {
                if(!best) {chosen=c.id;selected=unsigned(ready);return true;}
                if(c.hunger<=8&&c.thirst<=8) {
                    const auto tile=sites.sites[construction::field_slot(unsigned(ready))].tile;
                    move_toward(c,tile%width,tile/width);return true;
                }
            }
        }
        if(agriculture::farmer(research,c)&&c.hunger<=8&&c.thirst<=8) {
            int best=settlement::home_radius+1,near=-1;
            for(unsigned i=0;i<construction::field_count;++i) {
                const auto slot=construction::field_slot(i);
                if(!agriculture::viable_tending(w,sites,farms.crops[i],w.tick+1,slot)) continue;
                const auto tile=sites.sites[slot].tile;
                const int d=std::abs(int(c.x)-int(tile%width))+std::abs(int(c.y)-int(tile/width));
                if(d<best) {best=d;near=int(i);}
            }
            if(near>=0&&best>=agriculture::tending_radius) {
                const auto tile=sites.sites[construction::field_slot(unsigned(near))].tile;
                move_toward(c,tile%width,tile/width);return true;
            }
        }
        return settlement::return_home(w,sites,c,home);
    }
};
inline bool step(World& w,discovery::State& research,construction::State& sites,
                 storage::State& reserve,cooking::State& cooked,shelter::State& sheltered,
                 settlement::State& home,State& farms,irrigation::State& water,
                 Production& p,discovery::Event& re,construction::Event& ce,std::uint64_t cadence) noexcept {
    const bool busy=research.job.active||construction::active(sites)>=0;
    const auto deposits=reserve.deposited;
    const Work policy{research,sites,reserve,farms,home,cadence};
    if(!storage::step_with_policies(w,research,sites,reserve,p,re,ce,cadence,
        cooking::MealPolicy{research,sites,reserve,cooked},shelter::Recovery{sites,sheltered},
        policy,agriculture::Renewal{sites})) return false;
    if(policy.chosen&&!busy&&!research.job.active&&construction::active(sites)<0&&
       re.kind==discovery::EventKind::none&&ce.kind==construction::EventKind::none&&deposits==reserve.deposited) {
        const auto i=policy.selected;
        if(agriculture::work(w,research,sites,farms.crops[i],w.founders[policy.chosen-1],
           construction::field_slot(i))==agriculture::Task::harvest) p.food+=agriculture::harvest_yield;
    }
    irrigation::transfer(w,sites,water);
    return true;
}
}
