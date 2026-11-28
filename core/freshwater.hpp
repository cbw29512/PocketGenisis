#pragma once
#include "wildlife.hpp"
namespace genesis::freshwater {
// Accessible freshwater shoreline proxy, not an impassable water tile.
inline constexpr unsigned fish_cap=8,forage_cap=32;
struct Cell {std::uint8_t habitat{},fish{},forage{};};
static_assert(sizeof(Cell)==3,"compact aquatic cells");
struct State {
    std::array<Cell,std::size_t(width)*height> cells{};
    std::uint64_t initial{},births{},deaths{},caught{},forage_initial{},forage_grown{},forage_eaten{},last_pulse{};
    std::uint64_t water_deaths{},food_deaths{},migrated{};
};
// Creation only: quantized terrain supplies the habitat, no live RNG.
template<class Terrain>
inline void initialize(State& s,const Terrain& terrain) noexcept {
    s=State{};
    for(std::size_t i=0;i<s.cells.size();++i) {
        const auto& t=terrain.cells[i];
        if(t.elevation>105||t.moisture<140) continue;
        auto& c=s.cells[i];
        c={1,static_cast<std::uint8_t>(2+t.moisture%3),16};
        s.initial+=c.fish;s.forage_initial+=c.forage;
    }
}
inline std::uint64_t population(const State& s) noexcept {
    std::uint64_t n=0;for(const auto& c:s.cells)n+=c.fish;return n;
}
inline bool valid(const State& s) noexcept {
    std::uint64_t forage=0;
    for(const auto& c:s.cells) {
        if(c.habitat>1||c.fish>fish_cap||c.forage>forage_cap||
           (!c.habitat&&(c.fish||c.forage))) return false;
        forage+=c.forage;
    }
    return s.initial+s.births==population(s)+s.deaths+s.caught&&
           s.deaths==s.water_deaths+s.food_deaths&&
           s.forage_initial+s.forage_grown==forage+s.forage_eaten;
}
inline bool available(const State& s,std::size_t i) noexcept {
    return i<s.cells.size()&&s.cells[i].habitat&&s.cells[i].fish;
}
// Caller validates local actor eligibility. One fish gives one meal.
inline bool catch_one(State& s,std::size_t i) noexcept {
    if(!available(s,i)) return false;
    --s.cells[i].fish;++s.caught;return true;
}
// Share the terrestrial migration pattern: alternating checkerboard
// origins, cardinal neighbors, strictly better local support, actual
// bounded transfer. No moving fish through dry land or creating fish.
inline unsigned support(const World& w,const State& s,std::size_t i,bool cold) noexcept {
    const auto& c=s.cells[i];
    if(!c.habitat) return 0;
    const auto forage=unsigned(c.forage)*(cold?3u:1u);
    return std::min({fish_cap,unsigned(w.tiles[i].water)/4,forage});
}
inline void migrate(const World& w,State& s,bool cold) noexcept {
    const auto color=(w.tick/600)%2;
    for(std::size_t i=0;i<s.cells.size();++i) {
        auto& from=s.cells[i];
        const auto x=i%width,y=i/width;
        if(!from.habitat||!from.fish||(x+y)%2!=color) continue;
        const auto supported=support(w,s,i,cold);
        // Only threatened groups move. Keep fish in viable habitats
        // instead of forcing perpetual redistribution.
        if(from.fish<=supported) continue;
        std::size_t best=i;unsigned room=0;
        const auto consider=[&](std::size_t j) noexcept {
            const auto limit=support(w,s,j,cold);
            const auto occupied=unsigned(s.cells[j].fish);
            const auto free=limit>occupied?limit-occupied:0;
            if(free>room) {room=free;best=j;}
        };
        if(x) consider(i-1);
        if(x+1<width) consider(i+1);
        if(y) consider(i-width);
        if(y+1<height) consider(i+width);
        if(best==i) continue;
        const auto moved=std::min(room,unsigned(from.fish)-supported);
        from.fish-=moved;s.cells[best].fish+=moved;s.migrated+=moved;
    }
}
inline void pulse(const World& w,State& s,bool balanced=false) noexcept {
    constexpr std::uint64_t feeding=600,breeding=3600;
    if(!w.tick||w.tick%feeding||w.tick<=s.last_pulse) return;
    s.last_pulse=w.tick;
    constexpr unsigned rates[]{8,8,4,1};
    const bool cold=balanced&&season(w)==3;
    // Cold-season candidate: lower feeding demand, not free food.
    // One forage unit supports up to three fish per pulse in winter.
    const unsigned per_unit=cold?3u:1u;
    if(balanced) migrate(w,s,cold);
    for(std::size_t i=0;i<s.cells.size();++i) {
        auto& c=s.cells[i];if(!c.habitat) continue;
        // Drinking-water stock is a habitat proxy, not an extra debit.
        // Fish consume actual separately tracked aquatic forage.
        const auto water=w.tiles[i].water;
        const auto added=std::min<unsigned>(forage_cap-c.forage,
                                           water>=12?rates[season(w)]:0);
        c.forage+=added;s.forage_grown+=added;
        const auto supported=std::min<unsigned>(fish_cap,water/4);
        const auto wet=std::min(unsigned(c.fish),supported);
        const auto fed=std::min(wet,unsigned(c.forage)*per_unit);
        s.water_deaths+=c.fish-wet;s.food_deaths+=wet-fed;
        s.deaths+=c.fish-fed;c.fish=static_cast<std::uint8_t>(fed);
        const auto eaten=(fed+per_unit-1)/per_unit;
        c.forage-=eaten;s.forage_eaten+=eaten;
        // In the cold-season candidate breeding is dormant in winter.
        if(!cold&&w.tick%breeding==0) {
            const auto born=std::min({wildlife::offspring_request(fed,fish_cap),
                                     unsigned(c.forage),supported-fed});
            c.fish+=born;c.forage-=born;s.births+=born;s.forage_eaten+=born;
        }
    }
}
}
