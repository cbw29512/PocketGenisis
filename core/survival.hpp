#pragma once
#include "care.hpp"
#include "resource_search.hpp"
#include <algorithm>

namespace genesis {
// Shared authoritative survival transaction for every active citizen.
inline void act_survival(World& w,Citizen& f,Citizen* dependent=nullptr) noexcept {
    accrue_needs(f);
    const bool urgent=urgent_need(f);
    const bool water=f.thirst>=f.hunger;
    if(urgent&&consume_need(w,f,f.x,f.y)) return;
    if(!urgent&&give_care(w,f,dependent)) return;
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
inline bool step_survival(World& w) noexcept {
    if(w.tick==std::numeric_limits<std::uint64_t>::max() || !valid_population(w)) return false;
    const auto requests=prepare_care(w);
    auto act=[&](Citizen& c) {
        const auto slot=requests[static_cast<std::size_t>(c.id)];
        act_survival(w,c,slot?&w.descendants[slot-1]:nullptr);
    };
    for(auto& f:w.founders) act(f);
    for(std::size_t i=0;i<w.descendant_count;++i) {
        auto& c=w.descendants[i];
        if(life_stage(c,w.tick)!=LifeStage::infant) act(c);
    }
    for(std::size_t i=0;i<w.descendant_count;++i) {
        auto& c=w.descendants[i];
        if(life_stage(c,w.tick)==LifeStage::infant) follow_parent(w,c);
    }
    return advance_clock(w);
}
} // namespace genesis
