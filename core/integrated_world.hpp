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
    std::uint64_t initial_wild{};
};
struct Observation {
    Production production{};
    discovery::Event discovery_event{};
    construction::Event construction_event{};
};
// Expensive cross-system boundary audit; not an extra per-tick scan.
// A valid running world may have zero fish or animals.
inline bool valid(const State& s) noexcept {
    if(!s.founding.accepted||s.world.seed!=s.founding.accepted_seed||
       !valid_population(s.world)||!hydrology::valid(s.drainage,s.terrain)||
       !terrain_resources::valid(s.minerals)||!wildlife::valid(s.wild)||
       !freshwater::valid(s.fish)||!multi_field::valid(s.farms)||
       !livestock::valid(s.animals)) return false;
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
    bool ready() const noexcept {return ready_;}
    const State& state() const noexcept {return state_;}
    const Observation& observation() const noexcept {return observation_;}
    // Cadence is explicit and fixed at the caller's chosen test profile.
    // No repeated generation, observer input or duplicate ecology tick.
    bool step(std::uint64_t cadence) noexcept {
        if(!ready_||!cadence) return false;
        auto& s=state_;
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
