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
    bool productive(const World& w,Citizen& c) const noexcept {
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
        if(!chosen&&agriculture::farmer(research,c)) {
#ifdef PG_LIVESTOCK_PROBE
            // A known completed workplace is a bounded catalog lookup,
            // not a local exploration search. Walk back even from far away.
            int best=width+height,ready=-1;
#else
            int best=settlement::home_radius+1,ready=-1;
#endif
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
        return false;
    }
    // Separate work from idle locality so other actual economic tasks
    // can precede optional wandering/return-home without copying crop logic.
    bool locality(const World& w,Citizen& c) const noexcept {
        if(agriculture::farmer(research,c)&&c.hunger<=8&&c.thirst<=8) {
#ifdef PG_LIVESTOCK_PROBE
            int best=width+height,near=-1;
#else
            int best=settlement::home_radius+1,near=-1;
#endif
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
    bool operator()(const World& w,Citizen& c) const noexcept {
        if(productive(w,c)) return true;
        if(research.job.active||construction::active(sites)>=0) return false;
        return locality(w,c);
    }
};
// Extended work policies retain the existing selected crop transaction.
template<class Policy,class Food,class Renewal>
inline bool step_with_renewal(World& w,discovery::State& research,construction::State& sites,
                 storage::State& reserve,shelter::State& sheltered,
                 State& farms,irrigation::State& water,
                 Production& p,discovery::Event& re,construction::Event& ce,std::uint64_t cadence,
                 const Policy& policy,const Food& food,const Renewal& renewal) noexcept {
    const bool busy=research.job.active||construction::active(sites)>=0;
    const auto deposits=reserve.deposited;
    if(!storage::step_with_policies(w,research,sites,reserve,p,re,ce,cadence,
        food,shelter::Recovery{sites,sheltered},
        policy,renewal)) return false;
    if(policy.chosen&&!busy&&!research.job.active&&construction::active(sites)<0&&
       re.kind==discovery::EventKind::none&&ce.kind==construction::EventKind::none&&deposits==reserve.deposited) {
        const auto i=policy.selected;
        const auto before=farms.crops[i].food;
        if(agriculture::work(w,research,sites,farms.crops[i],w.founders[policy.chosen-1],
           construction::field_slot(i))==agriculture::Task::harvest) p.food+=farms.crops[i].food-before;
    }
    irrigation::transfer(w,sites,water);
    for(unsigned i=0;i<construction::field_count;++i)
        agriculture::recover_soil(w,sites,farms.crops[i],construction::field_slot(i));
    return true;
}
template<class Policy,class Food>
inline bool step_with_food_policy(World& w,discovery::State& research,construction::State& sites,
                 storage::State& reserve,shelter::State& sheltered,
                 State& farms,irrigation::State& water,
                 Production& p,discovery::Event& re,construction::Event& ce,std::uint64_t cadence,
                 const Policy& policy,const Food& food) noexcept {
    return step_with_renewal(w,research,sites,reserve,sheltered,farms,water,p,re,ce,cadence,
                            policy,food,agriculture::Renewal{sites});
}
template<class Policy>
inline bool step_with_policy(World& w,discovery::State& research,construction::State& sites,
                 storage::State& reserve,cooking::State& cooked,shelter::State& sheltered,
                 State& farms,irrigation::State& water,
                 Production& p,discovery::Event& re,construction::Event& ce,std::uint64_t cadence,
                 const Policy& policy) noexcept {
    return step_with_food_policy(w,research,sites,reserve,sheltered,farms,water,p,re,ce,cadence,
                                 policy,cooking::MealPolicy{research,sites,reserve,cooked});
}
inline bool step(World& w,discovery::State& research,construction::State& sites,
                 storage::State& reserve,cooking::State& cooked,shelter::State& sheltered,
                 settlement::State& home,State& farms,irrigation::State& water,
                 Production& p,discovery::Event& re,construction::Event& ce,std::uint64_t cadence) noexcept {
    return step_with_policy(w,research,sites,reserve,cooked,sheltered,farms,water,p,re,ce,cadence,
                            Work{research,sites,reserve,farms,home,cadence});
}
}
