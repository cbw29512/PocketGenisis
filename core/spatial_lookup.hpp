#pragma once
#include "world.hpp"
#include <algorithm>
#include <cstdlib>

namespace genesis::detail {
// Row-major tile IDs fit in 16 bits. All callers operate on validated positions.
static_assert(std::size_t(width)*height<65536);
inline std::uint16_t tile_key(const Citizen& c) noexcept {
    return static_cast<std::uint16_t>(std::size_t(c.y)*width+c.x);
}
// Enumerate clipped, inclusive row intervals of a Manhattan neighborhood.
// No wrapping across edges; a tile belongs to exactly one interval.
template<class Visit>
inline void local_rows(const Citizen& c,int radius,const Visit& visit) noexcept {
    for(int y=std::max(0,int(c.y)-radius);y<=std::min(int(height)-1,int(c.y)+radius);++y) {
        const int dx=radius-std::abs(y-int(c.y));
        const int first=y*width+std::max(0,int(c.x)-dx);
        const int last=y*width+std::min(int(width)-1,int(c.x)+dx);
        visit(static_cast<std::uint16_t>(first),static_cast<std::uint16_t>(last));
    }
}
// Bounded sorted multiset; duplicate tile keys represent distinct citizens.
// Snapshot stores only two bytes per citizen, not a second World or 6000 counters.
struct TileCounts {
    std::array<std::uint16_t,descendant_capacity+2> keys{};
    std::size_t count{};
    // Small direct-mapped phase cache: key includes radius, collisions replace
    // entries rather than affecting the authoritative sorted multiset.
    struct Cached { std::uint32_t key{}; unsigned total{}; };
    mutable std::array<Cached,128> cache{};
    void add(const Citizen& c) noexcept {keys[count++]=tile_key(c);}
    void sort() noexcept {std::sort(keys.begin(),keys.begin()+count);cache.fill(Cached{});}
    unsigned nearby(const Citizen& c,int radius) const noexcept {
        const auto key=std::uint32_t(tile_key(c))+1u+(std::uint32_t(radius)<<16);
        auto& entry=cache[key%cache.size()];
        if(entry.key==key) return entry.total;
        unsigned total=0;
        local_rows(c,radius,[&](std::uint16_t first,std::uint16_t last) noexcept {
            const auto end=keys.begin()+count;
            const auto lo=std::lower_bound(keys.begin(),end,first);
            const auto hi=std::upper_bound(lo,end,last);
            total+=static_cast<unsigned>(hi-lo);
        });
        entry={key,total};
        return total;
    }
};
}
