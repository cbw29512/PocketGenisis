#pragma once
#include "livestock_planning.hpp"
#include "wildlife_food.hpp"
#ifdef PG_MANURE_PROBE
#include "manure_work.hpp"
#endif

namespace genesis::livestock {
struct Work : multi_field::Work {
    const wildlife::Wildlife& wild;
    const State& animals;
    mutable std::uint64_t handler{};
#ifdef PG_MANURE_PROBE
    mutable std::uint64_t soil_worker{};
    mutable manure::Task soil_task{};
#endif
    bool operator()(const World& w,Citizen& c) const noexcept {
#ifdef PG_MANURE_PROBE
        if(soil_worker) return false;
        // Keep storage, construction and research priority. A depleted
        // field can attract a fertilizer worker before another planting.
        if(!soil_worker&&!handler&&!chosen&&
           !storage::ReserveWork{research,sites,reserve,cadence}(w,c)&&
           !research.job.active&&construction::active(sites)<0) {
            manure::Task task{};
            if(manure::seek(w,research,sites,farms,animals.organic,c,task)) {
                if(task!=manure::Task::none) {soil_worker=c.id;soil_task=task;}
                return true;
            }
        }
#endif
        if(productive(w,c)) return true;
        if(research.job.active||construction::active(sites)>=0) return false;
        if(!handler&&!chosen) {
            if(choose(w,research,sites,wild,animals,c,true).task!=Task::none) {
                handler=c.id;return true;
            }
            if(seek_work(w,research,sites,wild,animals,c,true)) return true;
        }
        return locality(w,c);
    }
};
template<class Renewal>
inline bool step_with_renewal(World& w,discovery::State& research,construction::State& sites,
                 storage::State& reserve,cooking::State& cooked,shelter::State& sheltered,
                 settlement::State& home,multi_field::State& farms,irrigation::State& water,
                 wildlife::Wildlife& wild,wildlife::Totals& ecology,State& animals,
                 Production& p,discovery::Event& re,construction::Event& ce,std::uint64_t cadence,
                 const Renewal& renewal,freshwater::State* fish=nullptr,bool fish_balance=false) noexcept {
    const bool busy=research.job.active||construction::active(sites)>=0;
    const auto deposits=reserve.deposited;
    const Work policy{{research,sites,reserve,farms,home,cadence},wild,animals};
    const wildlife::MealPolicy food{{research,sites,reserve,cooked},wild,ecology,fish};
    if(!multi_field::step_with_renewal(w,research,sites,reserve,sheltered,farms,water,p,re,ce,cadence,policy,food,renewal)) return false;
    mark_development(w,sites,wild);
    if(policy.handler&&!policy.chosen&&!busy&&!research.job.active&&construction::active(sites)<0&&
       re.kind==discovery::EventKind::none&&ce.kind==construction::EventKind::none&&deposits==reserve.deposited)
        work(w,research,sites,wild,animals,w.founders[policy.handler-1],true);
#ifdef PG_MANURE_PROBE
    if(policy.soil_worker&&!policy.handler&&!policy.chosen&&!busy&&
       !research.job.active&&construction::active(sites)<0&&
       re.kind==discovery::EventKind::none&&ce.kind==construction::EventKind::none&&deposits==reserve.deposited)
        manure::work(w,research,sites,farms,animals.organic,
                     w.founders[policy.soil_worker-1],policy.soil_task);
#endif
    pulse(w,sites,animals,wildlife::sustainable_candidate,true);
    wildlife::advance(w,wild,ecology,wildlife::predator_candidate,false);
    crop_loss::evaluate_fields(w,sites,farms);
    if(fish) freshwater::pulse(w,*fish,fish_balance);
    return true;
}
inline bool step(World& w,discovery::State& research,construction::State& sites,
                 storage::State& reserve,cooking::State& cooked,shelter::State& sheltered,
                 settlement::State& home,multi_field::State& farms,irrigation::State& water,
                 wildlife::Wildlife& wild,wildlife::Totals& ecology,State& animals,
                 Production& p,discovery::Event& re,construction::Event& ce,std::uint64_t cadence) noexcept {
    return step_with_renewal(w,research,sites,reserve,cooked,sheltered,home,farms,water,
                            wild,ecology,animals,p,re,ce,cadence,agriculture::Renewal{sites});
}
}
