#pragma once
#include "walkability.hpp"
namespace genesis::walkability {
// Creation-only resource report. Only land stocks are counted: shallow
// crossings connect regions but do not provide assumed harvestable resources.
struct Budget {
    std::uint32_t land_tiles{},food{},water{},wood{},stone{};
};
struct Assessment {Budget available{};bool same_region{},accepted{};};
// Thresholds are explicit caller policy, not hidden balancing constants.
inline Assessment assess_start(const World& world,const Map& map,
                               const Regions& regions,std::size_t a,std::size_t b,
                               const Budget& minimum) noexcept {
    Assessment result{};
    if(world.tick||!connected(regions,a,b)||
       map[a]!=Surface::land||map[b]!=Surface::land||a==b) return result;
    bool adjacent=false;
    hydrology::neighbors(a,[&](std::size_t j) noexcept {adjacent|=j==b;});
    if(!adjacent) return result;
    result.same_region=true;
    // Count actual stocks once, using the already computed region labels.
    for(std::size_t i=0;i<count;++i) {
        if(regions.label[i]!=regions.label[a]||map[i]!=Surface::land) continue;
        const auto& t=world.tiles[i];auto& s=result.available;
        ++s.land_tiles;s.food+=t.food;s.water+=t.water;s.wood+=t.wood;s.stone+=t.stone;
    }
    const auto& s=result.available;
    result.accepted=s.land_tiles>=minimum.land_tiles&&s.food>=minimum.food&&
        s.water>=minimum.water&&s.wood>=minimum.wood&&s.stone>=minimum.stone;
    return result;
}
}
