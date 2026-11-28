#pragma once
#include "livestock_planning.hpp"

namespace genesis::livestock {
struct Work : multi_field::Work {
    const wildlife::Wildlife& wild;
    const State& animals;
    mutable std::uint64_t handler{};
    bool operator()(const World& w,Citizen& c) const noexcept {
        if(productive(w,c)) return true;
        if(research.job.active||construction::active(sites)>=0) return false;
        if(!handler&&!chosen) {
            if(choose(w,research,sites,wild,animals,c).task!=Task::none) {
                handler=c.id;return true;
            }
            if(seek_work(w,research,sites,wild,animals,c)) return true;
        }
        return locality(w,c);
    }
};
inline bool step(World& w,discovery::State& research,construction::State& sites,
                 storage::State& reserve,cooking::State& cooked,shelter::State& sheltered,
                 settlement::State& home,multi_field::State& farms,irrigation::State& water,
                 wildlife::Wildlife& wild,wildlife::Totals& ecology,State& animals,
                 Production& p,discovery::Event& re,construction::Event& ce,std::uint64_t cadence) noexcept {
    const bool busy=research.job.active||construction::active(sites)>=0;
    const auto deposits=reserve.deposited;
    const Work policy{{research,sites,reserve,farms,home,cadence},wild,animals};
    if(!multi_field::step_with_policy(w,research,sites,reserve,cooked,sheltered,farms,water,p,re,ce,cadence,policy)) return false;
    mark_development(w,sites,wild);
    if(policy.handler&&!policy.chosen&&!busy&&!research.job.active&&construction::active(sites)<0&&
       re.kind==discovery::EventKind::none&&ce.kind==construction::EventKind::none&&deposits==reserve.deposited)
        work(w,research,sites,wild,animals,w.founders[policy.handler-1]);
    pulse(w,sites,animals,wildlife::sustainable_candidate);
    wildlife::advance(w,wild,ecology,wildlife::predator_candidate);
    crop_loss::evaluate_fields(w,sites,farms);
    return true;
}
}
