#pragma once
#include "construction.hpp"
#include "discovery_work.hpp"

namespace genesis::construction {
struct ReserveWork {
    const discovery::State& research;
    const State& sites;
    std::uint64_t cadence;
    bool operator()(const World& w,const Citizen& c) const noexcept {
        if(c.id<1||c.id>2||!discovery::available(c)) return false;
        const unsigned owner=static_cast<unsigned>(c.id-1);
        const auto k=active(sites);
        if(k>=0) return sites.sites[k].owner==owner&&sites.sites[k].tile==location(c);
        if(discovery::ReserveWork{research,cadence}(w,c)) return true;
        if(research.job.active) return false;
        for(unsigned j=0;j<kind_count;++j) if(can_start(w,research,sites,owner,j)) return true;
        return false;
    }
};
inline bool step(World& w,discovery::State& research,State& sites,Production& p,
                 discovery::Event& re,Event& ce,std::uint64_t cadence) noexcept {
    re={};ce={};
    const bool building=active(sites)>=0;
    if(!detail::step_world_observed(w,p,valid_population,[](TickPhase) noexcept {},
                                  ReserveWork{research,sites,cadence})) return false;
    if(building) ce=advance(w,research,sites);
    else {
        const bool researching=research.job.active;
        re=discovery::advance(w,research,cadence);
        if(!researching&&!research.job.active&&re.kind==discovery::EventKind::none)
            ce=advance(w,research,sites);
    }
    return true;
}
}
