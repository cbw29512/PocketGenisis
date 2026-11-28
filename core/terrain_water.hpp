#pragma once
#include "terrain_navigation.hpp"
namespace genesis::navigation {
// Creation-only equilibrium water candidate. The caller supplies fixed
// integer elevations and sea level. Depth is independent of resource stock.
// Interior basins fill only to their lowest spill saddle, capped by a
// provisional freshwater level. No rainfall, flow or volume ledger yet.
enum class WaterKind : std::uint8_t { dry, fresh, salt };
struct WaterMap {
    std::array<std::uint8_t,count> depth{};
    std::array<WaterKind,count> kind{};
};
struct WaterPolicy {
    std::uint8_t sea_level{105},freshwater_ceiling{125},wade_depth{2};
};
inline bool valid_policy(WaterPolicy p) noexcept {
    return p.freshwater_ceiling>=p.sea_level&&p.wade_depth>0;
}
inline bool boundary(std::size_t i) noexcept {
    return i%width==0||i%width==width-1||i<width||i>=count-width;
}
// Priority flood from the map perimeter. Reuse the indexed heap; distance
// is the minimum spill elevation along any path to the boundary.
inline bool generate_water(Terrain& terrain,WaterMap& water,Scratch& scratch,WaterPolicy p={}) noexcept {
    if(!valid_policy(p))return false;
    scratch.reset();
    for(std::size_t i=0;i<count;++i){
        if(!boundary(i))continue;
        scratch.distance[i]=terrain.elevation[i];
        scratch.changed(static_cast<std::uint16_t>(i));
    }
    while(scratch.used){
        const auto here=scratch.pop();
        hydrology::neighbors(here,[&](std::size_t next) noexcept {
            const auto candidate=std::max<unsigned>(scratch.distance[here],terrain.elevation[next]);
            if(candidate>=scratch.distance[next])return;
            scratch.distance[next]=static_cast<std::uint16_t>(candidate);
            scratch.changed(static_cast<std::uint16_t>(next));
        });
    }
    for(std::size_t i=0;i<count;++i){
        const unsigned ground=terrain.elevation[i];
        const bool sea=ground<p.sea_level&&scratch.distance[i]<p.sea_level;
        const unsigned level=sea?unsigned(p.sea_level):
            std::min<unsigned>(scratch.distance[i],p.freshwater_ceiling);
        const auto depth=level>ground?level-ground:0;
        water.depth[i]=static_cast<std::uint8_t>(depth);
        water.kind[i]=!depth?WaterKind::dry:sea?WaterKind::salt:WaterKind::fresh;
        terrain.surface[i]=surface(depth,p.wade_depth);
    }
    return true;
}
}
