#pragma once
// Isolated resource-generation candidate, not normal world initialization.
#include "terrain_probe.hpp"

namespace genesis::terrain_resources {
enum class Mineral : std::uint8_t {none,clay,copper,tin,iron,coal,gold,silver,gem,count};
struct Deposit {
    std::uint16_t remaining{};
    Mineral kind{};
    std::uint8_t discovered{}; // Hidden until genuine prospecting; no UI leak.
};
static_assert(sizeof(Deposit)==4,"bounded mineral map storage");
struct State {std::array<Deposit,std::size_t(width)*height> deposits{};};
// Relative initial availability by biome, applied to the existing seeded
// tile stocks. These are provisional ecological coefficients, not geology
// or a guarantee that all resources occur in every world.
struct Profile {unsigned food,wood,water,stone;};
inline constexpr std::array<Profile,5> profiles{{
    {85,55,100,45}, // lowland (not necessarily water)
    {100,40,80,60}, // grassland
    {90,100,85,45}, // forest
    {35,15,30,80},  // dryland
    {50,30,50,100}  // highland
}};
inline std::uint16_t scaled(std::uint16_t v,unsigned percent) noexcept {
    return static_cast<std::uint16_t>(unsigned(v)*percent/100);
}
inline Tile stock_for(Tile t,terrain_probe::Biome biome) noexcept {
    const auto& p=profiles[static_cast<unsigned>(biome)];
    return {scaled(t.food,p.food),scaled(t.wood,p.wood),
            scaled(t.water,p.water),scaled(t.stone,p.stone)};
}
// Stable five-cell patches create coherent local deposits rather than
// independent ore loot rolls. One dominant mineral per cell is a compact
// first candidate; actual amounts vary with elevation/moisture and seed.
inline Mineral select(terrain_probe::Biome biome,unsigned roll) noexcept {
    using B=terrain_probe::Biome;using M=Mineral;
    if(biome==B::lowland) return roll<35?M::clay:roll<43?M::coal:M::none;
    if(biome==B::highland) {
        if(roll<22) return M::iron;
        if(roll<39) return M::copper;
        if(roll<48) return M::tin;
        if(roll<56) return M::coal;
        if(roll<61) return M::silver;
        if(roll<64) return M::gold;
        if(roll<66) return M::gem;
        return M::none;
    }
    if(biome==B::dryland) return roll<12?M::copper:roll<20?M::iron:roll<23?M::gem:M::none;
    return roll<10?M::clay:roll<15?M::coal:roll<19?M::copper:M::none;
}
inline unsigned richness(Mineral m) noexcept {
    switch(m) {
        case Mineral::clay:return 100;
        case Mineral::iron:case Mineral::coal:return 80;
        case Mineral::copper:case Mineral::tin:return 55;
        case Mineral::silver:case Mineral::gold:return 20;
        case Mineral::gem:return 10;
        default:return 0;
    }
}
inline void generate(State& out,const terrain_probe::State& terrain,std::uint64_t seed) noexcept {
    constexpr unsigned patch=5;
    for(unsigned y=0;y<height;++y) for(unsigned x=0;x<width;++x) {
        const auto i=y*width+x;
        // Domain separation: deposit generation does not alter live RNG.
        const auto block=(y/patch)*((width+patch-1)/patch)+x/patch;
        Rng seam{seed^0x4d494e4552414c31ULL^std::uint64_t(block)};
        std::uint32_t roll{};seam.bounded(100,roll);
        const auto mineral=select(terrain.cells[i].biome,roll);
        Rng grade{seed^0x47524144454d494eULL^std::uint64_t(i)};
        std::uint32_t amount{};const auto limit=richness(mineral);
        if(limit) grade.bounded(limit,amount);
        out.deposits[i]={static_cast<std::uint16_t>(limit?amount+1:0),mineral,0};
    }
}
inline bool valid(const State& s) noexcept {
    for(const auto& d:s.deposits) {
        if(static_cast<unsigned>(d.kind)>=static_cast<unsigned>(Mineral::count)||
           d.discovered>1||d.remaining>richness(d.kind)||
           (d.kind==Mineral::none&&d.discovered)) return false;
    }
    return true;
}
// Deliberately explicit creation-only hook. Never run on a loaded world,
// and never call as a seasonal renewal or after resource depletion.
inline void initialize_stocks(World& w,const terrain_probe::State& terrain) noexcept {
    for(std::size_t i=0;i<w.tiles.size();++i)
        w.tiles[i]=stock_for(w.tiles[i],terrain.cells[i].biome);
}
}
