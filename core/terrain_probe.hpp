#pragma once
// Isolated generation candidate. Never called by normal world initialization.
#include "world.hpp"
#include "../third_party/fastnoiselite/FastNoiseLite.h"
#include <algorithm>
#include <climits>

namespace genesis::terrain_probe {
static_assert(sizeof(int)==4&&CHAR_BIT==8,"FastNoiseLite probe requires 32-bit int");
enum class Biome : std::uint8_t { lowland,grassland,forest,dryland,highland };
struct Cell {
    std::uint8_t elevation{},moisture{};
    Biome biome{};
};
static_assert(sizeof(Cell)==3,"bounded terrain probe storage");
struct State {std::array<Cell,std::size_t(width)*height> cells{};};
inline Biome classify(unsigned elevation,unsigned moisture) noexcept {
    if(elevation<85) return Biome::lowland;
    if(elevation>170) return Biome::highland;
    if(moisture<95) return Biome::dryland;
    if(moisture>150) return Biome::forest;
    return Biome::grassland;
}
inline std::uint8_t quantize(float n) noexcept {
    // Coarse integer state can eventually be saved rather than regenerated.
    return static_cast<std::uint8_t>(std::clamp((n+1.0f)*127.5f,0.0f,255.0f)+0.5f);
}
inline void generate(State& state,std::uint64_t seed) noexcept {
    // Domain-separated local generator: never consume the live world's streams.
    Rng rng{seed^0x5445525241494e31ULL};
    FastNoiseLite elevation(static_cast<int>(rng.next()&0x7fffffffULL));
    FastNoiseLite moisture(static_cast<int>(rng.next()&0x7fffffffULL));
    elevation.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    moisture.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    elevation.SetFrequency(0.045f);moisture.SetFrequency(0.07f);
    for(unsigned y=0;y<height;++y) for(unsigned x=0;x<width;++x) {
        auto& c=state.cells[y*width+x];
        c.elevation=quantize(elevation.GetNoise(float(x)+0.25f,float(y)+0.5f));
        c.moisture=quantize(moisture.GetNoise(float(x)+11.25f,float(y)+7.5f));
        c.biome=classify(c.elevation,c.moisture);
    }
}
inline std::uint64_t fingerprint(const State& state) noexcept {
    std::uint64_t h=14695981039346656037ULL;
    for(const auto& c:state.cells) {
        for(const auto v:{c.elevation,c.moisture,static_cast<std::uint8_t>(c.biome)}) {
            h^=v;h*=1099511628211ULL;
        }
    }
    return h;
}
}
