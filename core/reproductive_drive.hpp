#pragma once
#include "population.hpp"
#include "spatial_lookup.hpp"

namespace genesis::reproductive_drive {
// This is motivation, not a global population target or birth quota.
// Update only on the existing 600-tick conception opportunity.
inline constexpr unsigned radius=2,minimum_stock=12;
inline constexpr unsigned sparse_max=2,crowded_min=6;
inline bool viable(const World& w,const Citizen& c) noexcept {
    if(life_stage(c,w.tick)!=LifeStage::adult||c.health<50||
       c.hunger>24||c.thirst>24||c.x>=width||c.y>=height) return false;
#ifdef PG_FATIGUE_PROBE
    if(c.action==Action::rest||c.fatigue>=72) return false;
#endif
    const auto& t=w.tiles[detail::tile_key(c)];
    return t.food>=minimum_stock&&t.water>=minimum_stock;
}
inline int delta(bool suitable,unsigned neighbors) noexcept {
    if(!suitable) return -8;
    if(neighbors<=sparse_max) return 8;
    if(neighbors>=crowded_min) return -4;
    return 2;
}
inline void update(Citizen& c,bool adult,bool suitable,unsigned neighbors) noexcept {
    if(!adult) {c.reproductive_drive=0;return;}
    c.reproductive_drive=static_cast<std::uint8_t>(
        std::clamp(int(c.reproductive_drive)+delta(suitable,neighbors),0,100));
}
inline void advance(World& w) noexcept {
    // Reuse the disease engine's sorted local population snapshot.
    // Counts include all living ages; no second pairwise enumeration.
    detail::TileCounts local{};
    for(const auto& c:w.founders) if(c.health) local.add(c);
    for(std::size_t i=0;i<w.descendant_count;++i)
        if(w.descendants[i].health) local.add(w.descendants[i]);
    local.sort();
    const auto apply=[&](Citizen& c) noexcept {
        const auto adult=life_stage(c,w.tick)==LifeStage::adult;
        const auto nearby=local.nearby(c,radius);
        update(c,adult,viable(w,c),nearby?nearby-1:0);
    };
    for(auto& c:w.founders) apply(c);
    for(std::size_t i=0;i<w.descendant_count;++i) apply(w.descendants[i]);
}
inline unsigned chance(const Citizen& a,const Citizen& b) noexcept {
    // Bounded 5..45 percent; midpoint is the old 25% opportunity.
    // Pair selection, gestation and resource checks remain authoritative.
    const auto mean=(unsigned(a.reproductive_drive)+b.reproductive_drive)/2;
    return 5+40*std::min(100u,mean)/100;
}
inline void conceived(Citizen& c) noexcept {
    c.reproductive_drive=static_cast<std::uint8_t>(
        c.reproductive_drive>40?c.reproductive_drive-40:0);
}
}
