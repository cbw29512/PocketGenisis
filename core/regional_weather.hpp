#pragma once
#include "world.hpp"

namespace genesis::regional_weather {
// A compact fictional climate candidate: 60 regions, each 10x10 tiles,
// 30 simulated minutes per weather interval at the current 1s tick rate.
// (This is 30 real minutes equivalent, not an in-world solar clock.)
inline constexpr unsigned region_size=10;
inline constexpr unsigned columns=(width+region_size-1)/region_size;
inline constexpr unsigned rows=(height+region_size-1)/region_size;
inline constexpr std::uint64_t interval=1800;
enum class Kind : std::uint8_t { normal,dry,drought,rain };
struct Rates {unsigned food,water,wood;};
inline constexpr std::array<Rates,4> rates{{{100,100,100},{70,60,75},{40,20,50},{120,150,110}}};
inline constexpr Kind classify(unsigned roll,unsigned season_index) noexcept {
    // Summer is more drought-prone; winter growth still follows the
    // authoritative seasonal baseline. No weather can override winter zero.
    const unsigned drought=season_index==1?15:5;
    const unsigned dry=drought+20;
    if(roll<drought) return Kind::drought;
    if(roll<dry) return Kind::dry;
    if(roll>=80) return Kind::rain;
    return Kind::normal;
}
inline constexpr unsigned region(std::size_t tile) noexcept {
    return unsigned(tile/width)/region_size*columns+unsigned(tile%width)/region_size;
}
// Derived cache only. Rebuild from immutable seed, current tick and rule
// version after reload; no extra canonical RNG draws or saved timers.
struct Cache {
    std::uint64_t seed{};
    mutable std::uint64_t epoch=std::numeric_limits<std::uint64_t>::max();
    mutable std::array<Kind,columns*rows> regions{};
    explicit Cache(std::uint64_t world_seed) noexcept:seed(world_seed){}
    void refresh(std::uint64_t tick) const noexcept {
        const auto e=tick/interval;
        if(epoch==e) return;
        const auto s=unsigned((tick%ticks_per_year)/(ticks_per_year/4));
        Rng weather{seed^0x5745415448455231ULL^e};
        for(auto& k:regions) {
            std::uint32_t roll{};weather.bounded(100,roll);
            k=classify(roll,s);
        }
        epoch=e;
    }
    Kind at(std::uint64_t tick,std::size_t tile) const noexcept {
        refresh(tick);return regions[region(tile)];
    }
};
inline constexpr unsigned modify(unsigned base,unsigned percent) noexcept {
    return base*percent/100;
}
}
