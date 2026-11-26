#pragma once
#include "world.hpp"
#include "resource_search.hpp"
#include <algorithm>

namespace genesis {
// One bounded, integer-only transaction. No rendering or wall-clock input.
inline bool step_survival(World& w) noexcept {
    // Validate every agent before changing any state; caller logs failure.
    if (w.tick == std::numeric_limits<std::uint64_t>::max()) return false;
    for (std::size_t i=0; i<w.founders.size(); ++i) {
        const auto& f=w.founders[i];
        if (f.id!=i+1 || f.x>=width || f.y>=height ||
            f.hunger>100 || f.thirst>100 || f.personality[0]>100) return false;
    }
    for (auto& f : w.founders) {
        f.action=Action::idle;
        f.hunger=static_cast<std::uint8_t>(std::min(100, f.hunger+1));
        f.thirst=static_cast<std::uint8_t>(std::min(100, f.thirst+1));
        const bool urgent=f.hunger>=12 || f.thirst>=12;
        const bool water=f.thirst>=f.hunger;
        auto stock=[&](int x,int y)->std::uint16_t& {
            auto& t=w.tiles[static_cast<std::size_t>(y)*width+static_cast<std::size_t>(x)];
            return water?t.water:t.food;
        };
        if (urgent && stock(f.x,f.y)>0) {
            --stock(f.x,f.y); // Transfer exactly one unit to consumption.
            auto& need=water?f.thirst:f.hunger;
            need=static_cast<std::uint8_t>(std::max(0,need-12));
            f.action=water?Action::drink:Action::eat;
            continue;
        }
        constexpr int dx[]={-1,1,0,0}, dy[]={0,0,-1,1};
        std::array<int,4> candidates{};
        std::size_t count=0;
        for(int i=0;i<4;++i) {
            const int x=f.x+dx[i], y=f.y+dy[i];
            if(x>=0 && x<width && y>=0 && y<height)
                candidates[count++]=i;
        }
        int chosen=-1;
        if (urgent) {
            const auto target=find_resource(w,f.x,f.y,water);
            if(target.found) {
                // One cardinal step toward nearest stock; horizontal first.
                chosen=target.x<f.x?0:target.x>f.x?1:target.y<f.y?2:3;
            }
        } else if (count>0) {
            std::uint32_t roll{};
            w.streams[decisions].bounded(100,roll);
            if(roll<f.personality[0]) {
                w.streams[decisions].bounded(static_cast<std::uint32_t>(count),roll);
                chosen=candidates[roll];
            }
        }
        if(chosen>=0) {
            f.x=static_cast<std::uint16_t>(f.x+dx[chosen]);
            f.y=static_cast<std::uint16_t>(f.y+dy[chosen]);
            f.action=Action::move;
        }
    }
    return advance_clock(w);
}
} // namespace genesis
