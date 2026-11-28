#pragma once
#include "terrain_resources.hpp"
#include "freshwater.hpp"

namespace genesis::world_admission {
// Creation-only candidate. These minima qualify starting diversity,
// not future survival or economic sufficiency. No runtime stock floors.
inline constexpr unsigned max_attempts=32,min_biome_tiles=20,
    min_mineral_units=8,min_species_groups=2,min_fish_tiles=16;
inline constexpr std::size_t no_tile=std::size_t(width)*height;
struct Report {
    std::array<unsigned,5> biomes{};
    std::array<unsigned,static_cast<unsigned>(terrain_resources::Mineral::count)> ore{};
    std::array<unsigned,wildlife::species_count> groups{};
    std::array<unsigned,4> water_regions{},fish_regions{};
    unsigned fish_tiles{},founder_pairs{},wet_sinks{};
    std::size_t spawn_a{no_tile},spawn_b{no_tile};
    bool valid_data{},accepted{};
};
inline unsigned region(std::size_t i) noexcept {
    return unsigned(i%width>=width/2)+2u*unsigned(i/width>=height/2);
}
inline bool suitable(const Tile& t,terrain_probe::Biome b) noexcept {
    using B=terrain_probe::Biome;
    return b!=B::dryland&&b!=B::highland&&
           t.food>=40&&t.water>=40&&t.wood>=20&&t.stone>=12;
}
inline unsigned occupied_regions(const std::array<unsigned,4>& r) noexcept {
    unsigned n=0;for(auto count:r) n+=count>=8;return n;
}
// Read-only check of actual generated stocks/populations; use only on
// new candidates, never to judge or repair a civilization after launch.
inline Report inspect(const World& w,const terrain_probe::State& terrain,
                      const terrain_resources::State& minerals,
                      const wildlife::Wildlife& wild,const freshwater::State& fish,
                      const hydrology::State* network=nullptr) noexcept {
    Report r{};
    if(w.tick||!terrain_resources::valid(minerals)||!wildlife::valid(wild)||
       !freshwater::valid(fish)||(network&&!hydrology::valid(*network,terrain))) return r;
    Rng spawn{w.seed^0x535041574e5f3031ULL};
    for(std::size_t i=0;i<w.tiles.size();++i) {
        const auto& cell=terrain.cells[i];
        const auto b=static_cast<unsigned>(cell.biome);
        if(b>=r.biomes.size()||cell.biome!=terrain_probe::classify(cell.elevation,cell.moisture)) return Report{};
        ++r.biomes[b];
        const auto& t=w.tiles[i];
        const auto& d=minerals.deposits[i];
        if(d.discovered||wild.developed[i]) return Report{};
        r.ore[static_cast<unsigned>(d.kind)]+=d.remaining;
        if(t.water>=40) ++r.water_regions[region(i)];
        if(network&&bool(fish.cells[i].habitat)!=hydrology::wet(network->cells[i])) return Report{};
        if(fish.cells[i].fish>=2&&t.water>=12) {
            ++r.fish_tiles;++r.fish_regions[region(i)];
            if(network&&hydrology::lake(network->cells[i])) ++r.wet_sinks;
        }
        // Count existing breeding-size groups in locally provisioned
        // habitat. Predator prey-chain viability requires further tests.
        if(t.food>=12&&t.water>=12)
            for(unsigned k=0;k<wildlife::species_count;++k)
                r.groups[k]+=wild.tiles[i].count[k]>=2;
        // Adjacent safe founder pairs only. Reservoir selection avoids a
        // second map allocation and leaves the live RNG untouched.
        if(i%width+1>=width||!suitable(t,cell.biome)||
           !suitable(w.tiles[i+1],terrain.cells[i+1].biome)) continue;
        ++r.founder_pairs;
        std::uint32_t roll{};spawn.bounded(r.founder_pairs,roll);
        if(!roll) {r.spawn_a=i;r.spawn_b=i+1;}
    }
    r.valid_data=true;
    using B=terrain_probe::Biome;
    r.accepted=r.biomes[unsigned(B::forest)]>=min_biome_tiles&&
        r.biomes[unsigned(B::grassland)]>=min_biome_tiles&&
        occupied_regions(r.water_regions)>=2&&occupied_regions(r.fish_regions)>=2&&
        r.fish_tiles>=min_fish_tiles&&r.founder_pairs>0&&(!network||r.wet_sinks>=2);
    for(unsigned k=1;k<r.ore.size();++k) r.accepted&=r.ore[k]>=min_mineral_units;
    for(auto n:r.groups) r.accepted&=n>=min_species_groups;
    return r;
}
struct Selection {
    std::uint64_t requested_seed{},accepted_seed{};
    unsigned attempts{};
    Report report{};
    bool accepted{};
};
// Caller must supply disposable scratch state for an explicitly requested
// new-world creation. Failure leaves a rejected scratch candidate, never
// an authorized save. No call on boot/reload, no retries after acceptance.
inline Selection generate_candidate(World& w,terrain_probe::State& terrain,
                    terrain_resources::State& minerals,wildlife::Wildlife& wild,
                    freshwater::State& fish,std::uint64_t seed,
                    unsigned attempts=max_attempts,hydrology::State* network=nullptr) noexcept {
    Selection result{};result.requested_seed=seed;
    Rng alternatives{seed^0x574f524c4447454eULL};
    for(unsigned n=0;n<std::min(attempts,max_attempts);++n) {
        const auto candidate=n?alternatives.next():seed;
        initialize(w,candidate);
        terrain_probe::generate(terrain,candidate);
        terrain_resources::generate(minerals,terrain,candidate);
        terrain_resources::initialize_stocks(w,terrain);
        wildlife::initialize(wild,candidate);
        if(network) {
            hydrology::generate(*network,terrain);
            freshwater::initialize(fish,w,*network);
        } else freshwater::initialize(fish,terrain);
        result.attempts=n+1;
        result.report=inspect(w,terrain,minerals,wild,fish,network);
        if(!result.report.accepted) continue;
        result.accepted=true;result.accepted_seed=candidate;
        for(unsigned i=0;i<2;++i) {
            const auto tile=i?result.report.spawn_b:result.report.spawn_a;
            w.founders[i].x=static_cast<std::uint16_t>(tile%width);
            w.founders[i].y=static_cast<std::uint16_t>(tile/width);
        }
        return result;
    }
    return result;
}
}
