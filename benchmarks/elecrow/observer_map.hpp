#pragma once
#include "../../core/integrated_world.hpp"
namespace genesis::observer {
// Pure view derived from live state. No RNG, no writes, no world copy.
inline constexpr unsigned tile_px=8;
static_assert(width*tile_px==800&&height*tile_px==480);
inline constexpr std::uint16_t rgb(unsigned r,unsigned g,unsigned b) noexcept {
    return static_cast<std::uint16_t>(((r&248u)<<8)|((g&252u)<<3)|(b>>3));
}
struct TileView {
    std::uint16_t ground;
    bool water,fish,prey,predator,tree;
};
inline TileView view(const integrated::State& s,std::size_t i) noexcept {
    if(i>=s.world.tiles.size()) return {0,false,false,false,false,false};
    const auto& terrain=s.terrain.cells[i];
    const auto& stocks=s.world.tiles[i];
    const auto& animals=s.wild.tiles[i].count;
    const bool wet=hydrology::wet(s.drainage.cells[i]);
    std::uint16_t color=rgb(86,133,63);
    switch(terrain.biome) {
        case terrain_probe::Biome::lowland:color=rgb(89,141,76);break;
        case terrain_probe::Biome::grassland:color=rgb(117,157,68);break;
        case terrain_probe::Biome::forest:color=rgb(48,99,56);break;
        case terrain_probe::Biome::dryland:color=rgb(176,143,77);break;
        case terrain_probe::Biome::highland:color=rgb(116,114,111);break;
    }
    // Legacy maps contain potential wet habitat, not water depth. Keep
    // the underlying land visibly walkable; actual water surfaces will
    // use the same authoritative surface map as movement in a new ruleset.
    if(wet) color=stocks.water>=12?rgb(79,137,112):rgb(104,131,105);
    else if(stocks.food<12&&terrain.biome!=terrain_probe::Biome::highland)
        color=rgb(133,118,75);
    bool prey=false;
    for(std::size_t k=0;k<wildlife::wolf;++k) prey=prey||animals[k]!=0;
    return {color,wet,s.fish.cells[i].fish!=0,prey,
            animals[wildlife::wolf]!=0||animals[wildlife::bear]!=0,
            terrain.biome==terrain_probe::Biome::forest&&stocks.wood>=20};
}
// Visual marker colors are keyed to actual construction kinds, never hidden ore.
inline constexpr std::uint16_t site_color(construction::Kind k) noexcept {
    switch(k) {
        case construction::campfire:return rgb(255,151,44);
        case construction::hut:return rgb(190,125,64);
        case construction::store:return rgb(224,186,116);
        case construction::field:return rgb(206,200,65);
        case construction::channel:return rgb(108,197,237);
        case construction::enclosure:return rgb(155,108,71);
        default:return rgb(180,180,180);
    }
}
}
