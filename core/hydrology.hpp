#pragma once
#include "world.hpp"
#include <algorithm>
namespace genesis::hydrology {
// Compact integer drainage candidate. Strict downhill edges terminate
// at local sinks; flat areas are not yet depression-filled/spill routed.
inline constexpr std::uint16_t none=0xffffu;
inline constexpr unsigned channel_threshold=12;
inline constexpr std::size_t tile_count=std::size_t(width)*height;
static_assert(tile_count<none);
struct Cell {std::uint16_t downstream{none},area{1},runoff{};};
static_assert(sizeof(Cell)==6,"bounded drainage records");
struct State {std::array<Cell,tile_count> cells{};};
template<class Visit>
inline void neighbors(std::size_t i,const Visit& visit) noexcept {
    if(i%width) visit(i-1);
    if(i%width+1<width) visit(i+1);
    if(i>=width) visit(i-width);
    if(i+width<tile_count) visit(i+width);
}
inline bool wet(const Cell& c) noexcept {return c.runoff>=channel_threshold;}
inline bool lake(const Cell& c) noexcept {return wet(c)&&c.downstream==none;}
inline bool connected(const State& s,std::size_t i,std::size_t j) noexcept {
    if(i>=tile_count||j>=tile_count||!wet(s.cells[i])||!wet(s.cells[j])) return false;
    return s.cells[i].downstream==j||s.cells[j].downstream==i;
}
template<class Terrain>
inline std::uint16_t downhill(const Terrain& terrain,std::size_t i) noexcept {
    auto elevation=terrain.cells[i].elevation;
    std::uint16_t best=none;
    neighbors(i,[&](std::size_t j) noexcept {
        const auto h=terrain.cells[j].elevation;
        if(h<elevation) {elevation=h;best=static_cast<std::uint16_t>(j);}
    });
    return best;
}
// Each sufficiently moist tile contributes one unit of potential runoff,
// not actual water inventory. Accumulate in descending elevation order.
// One creation-only scratch linked bucket per tile, no recursion/heap.
template<class Terrain>
inline void generate(State& out,const Terrain& terrain) noexcept {
    std::array<std::uint16_t,256> heads{};heads.fill(none);
    std::array<std::uint16_t,tile_count> next{};
    for(std::size_t i=0;i<tile_count;++i) {
        const auto& t=terrain.cells[i];
        out.cells[i]={downhill(terrain,i),1,static_cast<std::uint16_t>(t.moisture>=100)};
        next[i]=heads[t.elevation];heads[t.elevation]=static_cast<std::uint16_t>(i);
    }
    for(int h=255;h>=0;--h)
        for(auto i=heads[unsigned(h)];i!=none;i=next[i]) {
            const auto& from=out.cells[i];
            if(from.downstream==none) continue;
            auto& to=out.cells[from.downstream];
            to.area+=from.area;to.runoff+=from.runoff;
        }
}
template<class Terrain>
inline bool valid(const State& s,const Terrain& terrain) noexcept {
    unsigned sinks_area=0,sinks_runoff=0,expected_runoff=0;
    for(std::size_t i=0;i<tile_count;++i) {
        const auto& c=s.cells[i];
        if(c.downstream!=downhill(terrain,i)||!c.area||c.area>tile_count||c.runoff>c.area) return false;
        unsigned area=1,runoff=terrain.cells[i].moisture>=100;
        expected_runoff+=runoff;
        neighbors(i,[&](std::size_t j) noexcept {
            if(s.cells[j].downstream==i) {area+=s.cells[j].area;runoff+=s.cells[j].runoff;}
        });
        if(c.area!=area||c.runoff!=runoff) return false;
        if(c.downstream==none) {sinks_area+=c.area;sinks_runoff+=c.runoff;}
    }
    return sinks_area==tile_count&&sinks_runoff==expected_runoff;
}
}
