#pragma once
#include <cstdint>
namespace genesis::soil {
// Nutrient capacity is not an inventory of edible food.
inline constexpr unsigned capacity=100,plant_cost=8,minimum_to_plant=24;
inline constexpr std::uint64_t recovery_interval=600;
struct State {
    std::uint8_t fertility{capacity};
    std::uint64_t spent{},restored{},last_recovery{};
#ifdef PG_MANURE_PROBE
    std::uint64_t amended{};
#endif
};
inline bool valid(const State& s) noexcept {
    return s.fertility<=capacity&&capacity+s.restored
#ifdef PG_MANURE_PROBE
        +s.amended
#endif
        ==s.spent+s.fertility;
}
inline bool can_plant(const State& s) noexcept {return s.fertility>=minimum_to_plant&&s.fertility<=capacity;}
inline bool plant(State& s) noexcept {
    if(!can_plant(s)) return false;
    s.fertility-=plant_cost;s.spent+=plant_cost;return true;
}
inline unsigned yield(const State& s) noexcept {
    return 2+(6u*s.fertility+99)/100;
}
// Recovery is a real elapsed fallow interval, not an inventory refill.
inline bool recover(State& s,std::uint64_t tick,std::uint64_t last_work,
                    bool fallow,unsigned water) noexcept {
    if(!fallow||water<12||s.fertility>=capacity||!tick||
       tick%recovery_interval||tick<=s.last_recovery||tick<last_work||
       tick-last_work<recovery_interval) return false;
    ++s.fertility;++s.restored;s.last_recovery=tick;return true;
}
}
