#pragma once
#include "terrain_water.hpp"
#include "walkable_admission.hpp"
namespace genesis::navigation {
// Exact same traversable edges as the route planner. Cliffs divide regions
// even where both tiles are dry. Labels are derived once at world admission.
inline void regions(const Terrain& terrain,walkability::Regions& out,
                    walkability::Scratch& scratch) noexcept {
    out.label.fill(walkability::blocked);out.components=0;
    for(std::size_t i=0;i<count;++i){
        if(!walkability::walkable(terrain.surface[i])||
           out.label[i]!=walkability::blocked)continue;
        const auto label=static_cast<std::uint16_t>(out.components++);
        std::size_t read=0,write=0;
        out.label[i]=label;scratch.queue[write++]=static_cast<std::uint16_t>(i);
        while(read<write){
            const auto here=scratch.queue[read++];
            hydrology::neighbors(here,[&](std::size_t next) noexcept {
                if(out.label[next]!=walkability::blocked||!cost(terrain,here,next))return;
                out.label[next]=label;scratch.queue[write++]=static_cast<std::uint16_t>(next);
            });
        }
    }
}
// The founders must share an actual dry, legal edge and have sufficient
// resources in their reachable component. Saltwater is never a drink stock.
inline walkability::Assessment assess(const World& world,const Terrain& terrain,
    const WaterMap& water,walkability::Regions& labels,walkability::Scratch& scratch,
    std::size_t a,std::size_t b,const walkability::Budget& minimum) noexcept {
    walkability::Assessment result{};
    if(world.tick||a>=count||b>=count||
       terrain.surface[a]!=walkability::Surface::land||
       terrain.surface[b]!=walkability::Surface::land||
       water.kind[a]!=WaterKind::dry||water.kind[b]!=WaterKind::dry||
       !cost(terrain,a,b))return result;
    regions(terrain,labels,scratch);
    if(!walkability::connected(labels,a,b))return result;
    result.same_region=true;
    for(std::size_t i=0;i<count;++i){
        if(labels.label[i]!=labels.label[a]||
           terrain.surface[i]!=walkability::Surface::land||
           water.kind[i]!=WaterKind::dry)continue;
        const auto& tile=world.tiles[i];auto& v=result.available;
        ++v.land_tiles;v.food+=tile.food;v.water+=tile.water;
        v.wood+=tile.wood;v.stone+=tile.stone;
    }
    const auto& v=result.available;
    result.accepted=v.land_tiles>=minimum.land_tiles&&v.food>=minimum.food&&
       v.water>=minimum.water&&v.wood>=minimum.wood&&v.stone>=minimum.stone;
    return result;
}
}
