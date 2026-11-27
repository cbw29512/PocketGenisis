#pragma once
#include "disease.hpp"

namespace genesis {
// Shared death provenance checks for RAM, archive segments and disk views.
inline bool valid_death_outcome(const DeathRecord& d) noexcept {
    const auto& c=d.citizen;
    if(c.id<3||d.death_tick<c.birth_tick||
       static_cast<unsigned>(d.cause)<1||
       static_cast<unsigned>(d.cause)>static_cast<unsigned>(DeathCause::parental_childbirth)||
       (d.cause==DeathCause::old_age?c.health==0:c.health!=0)) return false;
    if(d.cause==DeathCause::disease&&!is_ill(c)) return false;
    if((d.cause==DeathCause::childbirth)!=(c.birth_outcome==BirthOutcome::fatal)) return false;
    if(d.cause==DeathCause::childbirth&&d.death_tick!=c.birth_tick) return false;
    // The abstract parent model has no sex/gestational-parent field.
    if(d.cause==DeathCause::parental_childbirth&&
       life_stage(c,d.death_tick)<LifeStage::adult) return false;
    return true;
}
} // namespace genesis
