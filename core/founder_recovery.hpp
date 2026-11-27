#pragma once
#include "care.hpp"

namespace genesis {
// Derived condition, not a second health meter or an independently saved flag.
inline bool is_founder(const Citizen& c) noexcept {return c.id==1||c.id==2;}
inline constexpr unsigned founder_incapacitation_health=20;
inline constexpr std::uint64_t founder_crawl_period=4;
inline bool founder_incapacitated(const Citizen& c) noexcept {
    return is_founder(c)&&c.health<=founder_incapacitation_health;
}
inline bool founder_can_travel(const Citizen& c,std::uint64_t tick) noexcept {
    return !founder_incapacitated(c)||tick%founder_crawl_period==0;
}
// Nearby assistance consumes real stock, after each person's own action.
// No extra action for the helper, no resource creation, no genealogy change.
inline bool assist_founder(World& w,Citizen& helper,Citizen& patient) noexcept {
    if(!founder_incapacitated(patient)||!is_founder(helper)||helper.id==patient.id||
       helper.health<50||urgent_need(helper)||helper.action!=Action::idle||
       patient.action!=Action::idle||!urgent_need(patient)||distance_between(helper,patient)>1)
        return false;
    const bool water=patient.thirst>=patient.hunger;
    if(!consume_resource(w,patient,helper.x,helper.y,water)) {
        if((water?patient.hunger:patient.thirst)<12||
           !consume_resource(w,patient,helper.x,helper.y,!water)) return false;
    }
    helper.action=Action::care;
    return true;
}
} // namespace genesis
