#pragma once
#include "world.hpp"
#include <algorithm>

namespace genesis::wildlife {
// Disposable sidecar: explicit ecology state, no changes to existing World saves.
enum Species : std::size_t { deer,rabbit,boar,cattle,chicken,wolf,bear,species_count };
inline constexpr unsigned group_limit=64, meat_limit=100;
inline constexpr std::array<unsigned,5> yield{{6,2,5,8,2}};
struct WildlifeTile {
    std::array<std::uint16_t,species_count> count{};
    std::uint16_t meat{};
};
struct Wildlife {
    std::array<WildlifeTile,std::size_t(width)*height> tiles;
    // Explicit land-use input; a crowd is not automatically a city.
    std::array<std::uint8_t,std::size_t(width)*height> developed;
};
struct Totals {
    std::uint64_t births{}, deaths{}, hunted{}, meals{}, food{}, water{}, predation{}, spoiled{}, migrated{}, displaced_deaths{};
};
inline void initialize(Wildlife& state,std::uint64_t seed) noexcept {
    Rng rng{seed^0x57494c444c494645ULL};
    state.developed.fill(0);
    for(auto& tile:state.tiles) {
        tile=WildlifeTile{};
        for(std::size_t k=0;k<species_count;++k) {
            std::uint32_t roll{};
            rng.bounded(k<wolf?16u:64u,roll);
            if(!roll) tile.count[k]=k<wolf?4:2;
        }
    }
}
inline std::uint64_t population(const Wildlife& state) noexcept {
    std::uint64_t n=0;
    for(const auto& tile:state.tiles) for(auto count:tile.count) n+=count;
    return n;
}
inline bool valid(const Wildlife& state) noexcept {
    for(std::size_t i=0;i<state.tiles.size();++i) {
        const auto& tile=state.tiles[i];
        if(state.developed[i]>1||tile.meat>meat_limit) return false;
        for(auto count:tile.count) if(count>group_limit) return false;
    }
    return true;
}
// Consume shared vegetation/water; return the number that can be supported.
inline unsigned graze(Tile& resources,unsigned requested,Totals& out) noexcept {
    const auto fed=std::min({requested,unsigned(resources.food),unsigned(resources.water)});
    resources.food-=fed;resources.water-=fed;
    out.food+=fed;out.water+=fed;
    return fed;
}
// Predators can only consume actual living prey, using stable species order.
inline unsigned prey(WildlifeTile& tile,Tile& resources,unsigned requested,Totals& out) noexcept {
    const auto limit=std::min(requested,unsigned(resources.water));
    unsigned fed=0;
    for(std::size_t k=0;k<wolf&&fed<limit;++k) {
        const auto eaten=std::min(limit-fed,unsigned(tile.count[k]));
        tile.count[k]-=eaten;fed+=eaten;
    }
    resources.water-=fed;out.water+=fed;out.predation+=fed;out.deaths+=fed;
    return fed;
}
inline unsigned feed(WildlifeTile& tile,Tile& resources,std::size_t k,unsigned requested,Totals& out) noexcept {
    if(k<wolf) return graze(resources,requested,out);
    const auto fed=prey(tile,resources,requested,out);
    return k==bear?fed+graze(resources,requested-fed,out):fed;
}
inline void pulse(WildlifeTile& tile,Tile& resources,bool breeding,Totals& out) noexcept {
    if(tile.meat) {--tile.meat;++out.spoiled;}
    for(std::size_t k=0;k<species_count;++k) {
        const auto before=unsigned(tile.count[k]);
        if(!before) continue;
        const auto surviving=feed(tile,resources,k,before,out);
        tile.count[k]=static_cast<std::uint16_t>(surviving);
        out.deaths+=before-surviving;
        if(breeding&&surviving>=2) {
            const auto requested=std::min(group_limit-surviving,std::max(1u,surviving/4));
            const auto born=feed(tile,resources,k,requested,out);
            tile.count[k]+=born;out.births+=born;
        }
    }
}
inline void hunt(Citizen& c,WildlifeTile& tile,std::uint64_t tick,Totals& out,bool developed=false) noexcept {
    if(!c.health||c.action!=Action::idle||c.hunger<24||life_stage(c,tick)!=LifeStage::adult) return;
    if(!tile.meat&&!developed) {
        for(std::size_t k=0;k<wolf;++k) {
            if(!tile.count[k]||unsigned(tile.meat)+yield[k]>meat_limit) continue;
            --tile.count[k];tile.meat+=yield[k];++out.hunted;++out.deaths;
            break;
        }
    }
    if(tile.meat) {
        --tile.meat;++out.meals;
        c.hunger=static_cast<std::uint8_t>(c.hunger-12);
        c.action=Action::eat;
    }
}
} // namespace genesis::wildlife
#include "wildlife_movement.hpp"
namespace genesis::wildlife {
// Caller must first complete a validated human tick. No new RNG draws.
inline void advance(World& w,Wildlife& state,Totals& out) noexcept {
    auto act=[&](Citizen& c) noexcept {
        const auto i=std::size_t(c.y)*width+c.x;
        hunt(c,state.tiles[i],w.tick,out,state.developed[i]!=0);
    };
    for(auto& c:w.founders) act(c);
    for(std::size_t i=0;i<w.descendant_count;++i) act(w.descendants[i]);
    if(w.tick%60) return;
    migrate(w,state,out);
    for(std::size_t i=0;i<state.tiles.size();++i) {
        auto& tile=state.tiles[i];
        if(state.developed[i]) {
            // Habitat loss is accounted for explicitly; animals that cannot
            // escape this local pulse cannot feed or breed on developed land.
            for(auto& n:tile.count) {out.deaths+=n;out.displaced_deaths+=n;n=0;}
            if(tile.meat) {--tile.meat;++out.spoiled;}
        } else pulse(tile,w.tiles[i],w.tick%600==0,out);
    }
}
} // namespace genesis::wildlife
