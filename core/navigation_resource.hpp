#pragma once
#include "terrain_water.hpp"
namespace genesis::navigation {
enum class Need : std::uint8_t { food, water };
// Resource stock is distinct from water depth. Saltwater never satisfies
// thirst; submerged land stocks are not edible land harvests. Access to
// deep freshwater from a dry bank needs a separate physical transfer rule.
inline bool available(const World& w,const Terrain& t,const WaterMap& water,
                      std::size_t i,Need need) noexcept {
    if(i>=count||!walkability::walkable(t.surface[i]))return false;
    const auto kind=water.kind[i];
    const bool dry=kind==WaterKind::dry&&!water.depth[i]&&t.surface[i]==walkability::Surface::land;
    const bool fresh=kind==WaterKind::fresh&&water.depth[i]&&t.surface[i]==walkability::Surface::shallow;
    if(need==Need::food)return dry&&w.tiles[i].food>0;
    if(need==Need::water)return (dry||fresh)&&w.tiles[i].water>0;
    return false;
}
// Search by actual travel turns, not Manhattan distance. No stock is
// consumed or reserved by searching; the eventual action must recheck it.
inline GoalRoute seek(const World& w,const Terrain& t,const WaterMap& water,
                      std::size_t from,Need need,Scratch& scratch,unsigned budget) noexcept {
    return route_to(t,from,scratch,[&](std::size_t i) noexcept {
        return available(w,t,water,i,need);
    },budget);
}
}
