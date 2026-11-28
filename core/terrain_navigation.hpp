#pragma once
#include "walkability.hpp"
#include <algorithm>
namespace genesis::navigation {
// New-ruleset candidate. Immutable topology is an explicit input, never
// inferred from food/water stocks or retroactively applied to old saves.
enum class Cover : std::uint8_t { open, forest, highland };
struct Terrain {
    walkability::Map surface{};
    std::array<std::uint8_t,walkability::count> elevation{};
    std::array<Cover,walkability::count> cover{};
};
inline constexpr std::uint16_t unreachable=0xffffu;
inline constexpr unsigned cliff_rise=32,steep_rise=12,gentle_rise=4;
// Integer movement costs are provisional tuning values, not physical metres.
// Direction matters: ascending is harder than descending. Cliffs prohibit
// either direction until an explicit future infrastructure edge exists.
inline unsigned cost(const Terrain& t,std::size_t from,std::size_t to) noexcept {
    if(!walkability::can_step(t.surface,from,to))return 0;
    const int rise=int(t.elevation[to])-int(t.elevation[from]);
    const auto slope=unsigned(rise<0?-rise:rise);
    if(slope>cliff_rise)return 0;
    if(unsigned(t.cover[from])>unsigned(Cover::highland)||
       unsigned(t.cover[to])>unsigned(Cover::highland))return 0;
    unsigned ticks=1;
    if(t.surface[from]==walkability::Surface::shallow||
       t.surface[to]==walkability::Surface::shallow)ticks=3;
    else if(t.cover[to]==Cover::forest)ticks=2;
    else if(t.cover[to]==Cover::highland)ticks=3;
    if(slope>=steep_rise)ticks+=rise>0?3:2;
    else if(slope>=gentle_rise)ticks+=rise>0?2:1;
    return ticks;
}
// Explicit physical depth in quantized elevation units. Only use on a
// newly admitted map or explicitly migrated ruleset, never current saves.
inline walkability::Surface surface(unsigned depth,unsigned wade_depth) noexcept {
    return !depth?walkability::Surface::land:
        depth<=wade_depth?walkability::Surface::shallow:walkability::Surface::deep;
}
inline constexpr auto count=walkability::count;
static_assert((count-1)*6<unreachable,"route distance fits 16-bit storage");
// One caller-owned workspace, reusable across agents, no heap allocation in
// the pathfinder. Indexed heap holds at most one entry per tile.
struct Scratch {
    std::array<std::uint16_t,count> distance{},first{},heap{},position{};
    std::size_t used{};
    bool less(std::uint16_t a,std::uint16_t b) const noexcept {
        return distance[a]<distance[b]||(distance[a]==distance[b]&&a<b);
    }
    void swap_at(std::size_t a,std::size_t b) noexcept {
        std::swap(heap[a],heap[b]);position[heap[a]]=a;position[heap[b]]=b;
    }
    void reset() noexcept {
        distance.fill(unreachable);first.fill(unreachable);position.fill(unreachable);used=0;
    }
    void changed(std::uint16_t tile) noexcept {
        auto i=std::size_t(position[tile]);
        if(i==unreachable){i=used++;heap[i]=tile;position[tile]=i;}
        while(i){const auto p=(i-1)/2;if(!less(heap[i],heap[p]))break;swap_at(i,p);i=p;}
    }
    std::uint16_t pop() noexcept {
        const auto result=heap[0];--used;position[result]=unreachable;
        if(used){
            heap[0]=heap[used];position[heap[0]]=0;std::size_t i=0;
            while(i*2+1<used){
                auto child=i*2+1;
                if(child+1<used&&less(heap[child+1],heap[child]))++child;
                if(!less(heap[child],heap[i]))break;
                swap_at(i,child);i=child;
            }
        }
        return result;
    }
};
struct Route {
    std::uint16_t next{unreachable},total{unreachable};
    bool found{};
};
// Bounded deterministic Dijkstra search. A positive edge cost ensures a
// settled tile cannot improve later; stable tile ordering resolves ties.
inline Route route(const Terrain& terrain,std::size_t from,std::size_t to,Scratch& s) noexcept {
    if(from>=count||to>=count||!walkability::walkable(terrain.surface[from])||
       !walkability::walkable(terrain.surface[to]))return {};
    if(from==to)return {static_cast<std::uint16_t>(from),0,true};
    s.reset();s.distance[from]=0;s.changed(static_cast<std::uint16_t>(from));
    while(s.used){
        const auto here=s.pop();
        if(here==to)return {s.first[here],s.distance[here],true};
        hydrology::neighbors(here,[&](std::size_t next) noexcept {
            const auto edge=cost(terrain,here,next);
            if(!edge)return;
            const unsigned candidate=unsigned(s.distance[here])+edge;
            if(candidate>=s.distance[next])return;
            s.distance[next]=static_cast<std::uint16_t>(candidate);
            s.first[next]=here==from?static_cast<std::uint16_t>(next):s.first[here];
            s.changed(static_cast<std::uint16_t>(next));
        });
    }
    return {};
}
}
