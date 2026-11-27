#pragma once
#include "world.hpp"

namespace genesis {
// A resolved historical identity; the pointer is borrowed for this validation only.
struct PersonRecord {
    const Citizen* person{};
    bool dead{};
    std::uint64_t death_tick{};
};
template<class Lookup> inline bool valid_genealogy(const Citizen& c,std::uint64_t observed_tick,
                                    std::uint64_t next_id,const Lookup& lookup) noexcept {
    auto parent_valid=[&](std::uint64_t id) {
        if(id==1||id==2) return true;
        if(id<3||id>=c.id) return false;
        const auto p=lookup(id);
        return p.person&&p.person->birth_tick<=c.birth_tick&&
               c.birth_tick-p.person->birth_tick>=18*ticks_per_year&&
               (!p.dead||p.death_tick>=c.birth_tick);
    };
    if(!parent_valid(c.parent1)||!parent_valid(c.parent2)) return false;
    if(!c.guardian_id) return c.guardian_since_tick==0;
    const auto id=c.guardian_id,since=c.guardian_since_tick;
    if(id>=next_id||id==c.id||id==c.parent1||id==c.parent2||c.parent1<=2||
       since<c.birth_tick||since>observed_tick||since-c.birth_tick>=2*ticks_per_year) return false;
    for(auto parent:{c.parent1,c.parent2}) {
        const auto p=lookup(parent);
        if(!p.person||!p.dead||p.death_tick>since) return false;
    }
    if(id<=2) return true;
    const auto g=lookup(id);
    return g.person&&g.person->birth_tick<=since&&since-g.person->birth_tick>=18*ticks_per_year&&
           (!g.dead||g.death_tick>=since);
}
} // namespace genesis
