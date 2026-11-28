#pragma once
#include "integrated_profile.hpp"
namespace genesis::integrated {
// Own each existing subsystem exactly once; no new simulation rules.
// This is the disposable integration candidate, not a save format.
struct State {
    World world{};
    terrain_probe::State terrain{};
    terrain_resources::State minerals{};
    hydrology::State drainage{};
    wildlife::Wildlife wild{};
    freshwater::State fish{};
    discovery::State knowledge{};
    construction::State sites{};
    storage::State reserve{};
    cooking::State cooked{};
    shelter::State sheltered{};
    settlement::State home{};
    multi_field::State farms{};
    irrigation::State water{};
    livestock::State animals{};
    wildlife::Totals ecology{};
    world_admission::Selection founding{};
    std::uint64_t initial_wild{},cadence{};
};
struct Observation {
    Production production{};
    discovery::Event discovery_event{};
    construction::Event construction_event{};
};
// Expensive cross-system boundary audit; not an extra per-tick scan.
// A valid running world may have zero fish or animals.
inline bool valid(const State& s) noexcept {
    if(!s.founding.accepted||!s.founding.report.accepted||!s.founding.report.valid_data||
       !s.founding.attempts||s.founding.attempts>world_admission::max_attempts||
       s.founding.report.spawn_a>=s.world.tiles.size()||
       s.founding.report.spawn_b>=s.world.tiles.size()||
       (s.world.tick&&!s.cadence)||s.world.seed!=s.founding.accepted_seed||
       !valid_population(s.world)||!hydrology::valid(s.drainage,s.terrain)||
       !terrain_resources::valid(s.minerals)||!wildlife::valid(s.wild)||
       !freshwater::valid(s.fish)||!multi_field::valid(s.farms)||
       !livestock::valid(s.animals)) return false;
    if(!storage::valid(s.reserve)||!cooking::valid(s.cooked)||
       !settlement::valid(s.home)||!irrigation::valid(s.water)) return false;
    const auto tick=s.world.tick;
    const auto& k=s.knowledge;
    constexpr unsigned mask=(1u<<discovery::tech_count)-1u;
    if((k.known[0]&~mask)||(k.known[1]&~mask)||
       k.job.owner>=2||k.job.tech>=discovery::tech_count||
       k.job.work>discovery::work_required||
       (k.job.active&&k.job.work>=discovery::work_required)||
       k.job.last_work_tick>tick||k.last_opportunity>tick||k.last_teaching>tick||
       k.attempts!=k.successes+k.failures+k.abandoned+unsigned(k.job.active)) return false;
    for(const auto& r:k.first) if(r.founder>2||r.tick>tick||bool(r.tick)!=bool(r.founder)) return false;
    unsigned building=0;
    for(unsigned i=0;i<construction::site_count;++i) {
        const auto& site=s.sites.sites[i];
        if(unsigned(site.stage)>unsigned(construction::Stage::abandoned)||
           site.tile>=s.world.tiles.size()||site.owner>=2||
           site.work>construction::blueprints[construction::kind_at(i)].work||
           site.started>tick||site.finished>tick||site.last_work>tick) return false;
        building+=site.stage==construction::Stage::building;
        if(site.stage==construction::Stage::building&&
           site.work>=construction::blueprints[construction::kind_at(i)].work) return false;
    }
    if(building>1||s.water.last_transfer>tick||s.animals.last_pulse>tick||
       s.fish.last_pulse>tick) return false;
    std::uint64_t amended=0;
    for(const auto& crop:s.farms.crops) {
        if(crop.last_work>tick||crop.dry_since>tick||crop.nutrients.last_recovery>tick) return false;
        amended+=crop.nutrients.amended;
    }
    if(amended!=s.animals.organic.applied*manure::nutrient_value) return false;
    std::uint64_t meat=0;
    for(std::size_t i=0;i<s.world.tiles.size();++i) {
        if(bool(s.fish.cells[i].habitat)!=hydrology::wet(s.drainage.cells[i])) return false;
        meat+=s.wild.tiles[i].meat;
    }
    return s.initial_wild+s.ecology.births==wildlife::population(s.wild)+
               s.ecology.deaths+livestock::captures(s.animals)&&
           s.ecology.meat_created+s.animals.meat_produced==
               meat+s.ecology.meals+s.ecology.spoiled;
}
class Simulation {
    State state_{};
    regional_weather::Cache climate_{0};
    Observation observation_{};
    bool attempted_{},ready_{};
public:
    Simulation()=default;
    Simulation(const Simulation&)=delete;
    Simulation& operator=(const Simulation&)=delete;
    // Explicit one-shot creation; even at tick zero cannot overwrite an
    // admitted world. Failed scratch instances cannot advance.
    bool create(std::uint64_t seed,unsigned attempts=world_admission::max_attempts) noexcept {
        if(attempted_) return false;
        attempted_=true;
        auto& s=state_;
        s.founding=world_admission::generate_candidate(s.world,s.terrain,s.minerals,s.wild,
                      s.fish,seed,attempts,&s.drainage);
        if(!s.founding.accepted) return false;
        s.initial_wild=wildlife::population(s.wild);
        climate_=regional_weather::Cache{s.world.seed};
        ready_=valid(s);
        return ready_;
    }
    // Restore only into a fresh owner after full decoded-state validation.
    // Never regenerate terrain, reset a ready world or re-seed its streams.
    bool restore(const State& s,const Observation& o) noexcept {
        if(attempted_) return false;
        attempted_=true;
        if(!valid(s)) return false;
        state_=s;observation_=o;
        climate_=regional_weather::Cache{s.world.seed};
        ready_=true;return true;
    }
    bool ready() const noexcept {return ready_;}
    const State& state() const noexcept {return state_;}
    const Observation& observation() const noexcept {return observation_;}
    // Bind cadence on the first successful step; restored worlds cannot
    // silently change rules at the next call.
    // No repeated generation, observer input or duplicate ecology tick.
    bool step(std::uint64_t cadence) noexcept {
        if(!ready_||!cadence) return false;
        auto& s=state_;
        if(s.cadence&&s.cadence!=cadence) return false;
        s.cadence=cadence;
        const terrain_renewal::Policy policy{s.terrain,{s.sites},s.wild,&climate_};
        if(!livestock::step_with_renewal(s.world,s.knowledge,s.sites,s.reserve,s.cooked,
             s.sheltered,s.home,s.farms,s.water,s.wild,s.ecology,s.animals,
             observation_.production,observation_.discovery_event,observation_.construction_event,
             cadence,policy,&s.fish,true,&s.drainage)) {
            ready_=false;
            return false;
        }
        return true;
    }
};
}
