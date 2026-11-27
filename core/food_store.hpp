#pragma once
#include "construction_work.hpp"

namespace genesis::storage {
inline constexpr unsigned capacity=64,local_surplus=24;
struct State {
    unsigned balance{};
    std::uint64_t deposited{},withdrawn{},spoiled{};
};
inline bool valid(const State& s) noexcept {
    return s.balance<=capacity&&s.deposited==s.withdrawn+s.spoiled+s.balance;
}
inline bool nearby(const construction::State& sites,const Citizen& c) noexcept {
    const auto& b=sites.sites[construction::store];
    return b.stage==construction::Stage::complete&&
        std::abs(int(c.x)-int(b.tile%width))+std::abs(int(c.y)-int(b.tile/width))<=1;
}
inline bool can_deposit(const World& w,const construction::State& sites,const State& s,const Citizen& c) noexcept {
    return c.id>=1&&c.id<=2&&discovery::available(c)&&nearby(sites,c)&&s.balance<capacity&&
        w.tiles[construction::location(c)].food>local_surplus;
}
inline bool deposit(World& w,const construction::State& sites,State& s,Citizen& c) noexcept {
    if(!can_deposit(w,sites,s,c)) return false;
    --w.tiles[construction::location(c)].food;++s.balance;++s.deposited;
    return true;
}
struct ReserveFood {
    const construction::State& sites;
    State& reserve;
    bool operator()(World& w,Citizen& c) const noexcept {
        // The survival phase invokes this only after local food was unavailable.
        if(!reserve.balance||c.hunger<12||c.thirst>=c.hunger||
           c.action!=Action::idle||!nearby(sites,c)||w.tiles[construction::location(c)].food) return false;
        --reserve.balance;++reserve.withdrawn;satisfy_need(c,false);return true;
    }
};
inline void spoil(std::uint64_t tick,State& s) noexcept {
    if(tick&&tick%3600==0&&s.balance) {--s.balance;++s.spoiled;}
}
struct ReserveWork {
    const discovery::State& research;
    const construction::State& sites;
    const State& reserve;
    std::uint64_t cadence;
    bool operator()(const World& w,const Citizen& c) const noexcept {
        if(construction::ReserveWork{research,sites,cadence}(w,c)) return true;
        if(research.job.active||construction::active(sites)>=0) return false;
        return can_deposit(w,sites,reserve,c);
    }
};
inline bool step(World& w,discovery::State& research,construction::State& sites,State& reserve,
                 Production& p,discovery::Event& re,construction::Event& ce,std::uint64_t cadence) noexcept {
    const bool busy=research.job.active||construction::active(sites)>=0;
    if(!construction::step_with_policy(w,research,sites,p,re,ce,cadence,
            ReserveWork{research,sites,reserve,cadence},ReserveFood{sites,reserve})) return false;
    if(!busy&&!research.job.active&&construction::active(sites)<0&&
        re.kind==discovery::EventKind::none&&ce.kind==construction::EventKind::none) {
        // One discretionary deposit per world tick; no overlapping research/build work.
        for(auto& c:w.founders) if(deposit(w,sites,reserve,c)) break;
    }
    spoil(w.tick,reserve);return true;
}
}
