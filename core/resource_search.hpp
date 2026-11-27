#pragma once
#include "world.hpp"
#include <cstdlib>

namespace genesis {
// Maximum Manhattan radius. At most 144 candidate tiles, no heap allocation.
inline constexpr int search_radius=8;
struct ResourceTarget {
    int x{}, y{};
    bool found{};
};
// Open terrain only: impassable tiles will require a pathfinding contract.
inline ResourceTarget find_resource(const World& w, int x, int y, bool water) noexcept {
    if(x<0 || x>=width || y<0 || y>=height) return {};
    // Nearest nonempty tile wins; stable row-major order breaks ties.
    // Searching successive rings also allows crossing a depleted local tile.
    for(int distance=1;distance<=search_radius;++distance) {
        for(int dy=-distance;dy<=distance;++dy) {
            const int dx=distance-std::abs(dy);
            const int yy=y+dy;
            if(yy<0 || yy>=height) continue;
            for(int side=0;side<(dx==0?1:2);++side) {
                const int xx=x+(side==0?-dx:dx);
                if(xx<0 || xx>=width) continue;
                const auto& t=w.tiles[static_cast<std::size_t>(yy)*width+static_cast<std::size_t>(xx)];
                if((water?t.water:t.food)>0) return {xx,yy,true};
            }
        }
    }
    return {};
}
// Only desperate founders use this bounded fallback after local search fails.
inline ResourceTarget find_distant_resource(const World& w,int x,int y,bool water) noexcept {
    if(x<0||x>=width||y<0||y>=height) return {};
    ResourceTarget best{};
    int best_distance=width+height;
    for(int yy=0;yy<height;++yy) {
        for(int xx=0;xx<width;++xx) {
            const auto& t=w.tiles[static_cast<std::size_t>(yy)*width+static_cast<std::size_t>(xx)];
            const int distance=std::abs(xx-x)+std::abs(yy-y);
            if((water?t.water:t.food)>0&&distance<best_distance) {
                best={xx,yy,true};
                best_distance=distance;
            }
        }
    }
    return best;
}
} // namespace genesis
