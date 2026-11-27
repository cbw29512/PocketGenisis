#pragma once
#include "care.hpp"
#include "resource_search.hpp"
#include <algorithm>

namespace genesis {
inline void move_toward(Citizen& c,int x,int y) noexcept {
    if(c.x!=x) c.x=static_cast<std::uint16_t>(int(c.x)+(c.x<x?1:-1));
    else if(c.y!=y) c.y=static_cast<std::uint16_t>(int(c.y)+(c.y<y?1:-1));
    else return;
    c.action=Action::move;
}
// Shared authoritative survival transaction for every active citizen.
inline void act_survival(World& w,Citizen& f,Citizen* dependent=nullptr) noexcept {
    accrue_needs(f);
    const bool urgent=urgent_need(f);
    const bool water=f.thirst>=f.hunger;
    const bool founder=f.id==1||f.id==2;
    if(urgent) {
        if(consume_need(w,f,f.x,f.y)) return;
        auto target=find_resource(w,f.x,f.y,water);
        if(target.found) { move_toward(f,target.x,target.y);return; }
        // A missing primary resource must not prevent meeting another urgent need.
        const bool secondary_urgent=(water?f.hunger:f.thirst)>=12;
        if(secondary_urgent) {
            if(consume_resource(w,f,f.x,f.y,!water)) return;
            target=find_resource(w,f.x,f.y,!water);
            if(target.found) { move_toward(f,target.x,target.y);return; }
        }
        if(founder) {
            target=find_distant_resource(w,f.x,f.y,water);
            if(!target.found&&secondary_urgent)
                target=find_distant_resource(w,f.x,f.y,!water);
            if(target.found) move_toward(f,target.x,target.y);
        }
        return;
    }
    if(give_care(w,f,dependent)) return;
    // Reunite founders only after survival and care; never force conception.
    if(founder) {
        const auto& other=w.founders[f.id==1?1:0];
        if(distance_between(f,other)>2) {
            if(f.id==2) move_toward(f,other.x,other.y);
            return;
        }
    }
    constexpr int dx[]={-1,1,0,0},dy[]={0,0,-1,1};
    std::array<int,4> candidates{};
    std::size_t count=0;
    for(int i=0;i<4;++i) {
        const int x=f.x+dx[i],y=f.y+dy[i];
        if(x>=0&&x<width&&y>=0&&y<height) candidates[count++]=i;
    }
    if(count) {
        std::uint32_t roll{};
        w.streams[decisions].bounded(100,roll);
        if(roll<f.personality[0]) {
            w.streams[decisions].bounded(static_cast<std::uint32_t>(count),roll);
            const auto chosen=candidates[roll];
            move_toward(f,int(f.x)+dx[chosen],int(f.y)+dy[chosen]);
        }
    }
}
inline bool step_survival(World& w) noexcept {
    if(w.tick==std::numeric_limits<std::uint64_t>::max()||!valid_population(w)) return false;
    const auto requests=prepare_care(w);
    auto act=[&](Citizen& c,std::size_t live_index) {
        const auto slot=requests[live_index];
        act_survival(w,c,slot?&w.descendants[slot-1]:nullptr);
    };
    for(std::size_t i=0;i<w.founders.size();++i) act(w.founders[i],i);
    for(std::size_t i=0;i<w.descendant_count;++i) {
        auto& c=w.descendants[i];
        if(life_stage(c,w.tick)!=LifeStage::infant) act(c,i+2);
    }
    for(std::size_t i=0;i<w.descendant_count;++i) {
        auto& c=w.descendants[i];
        if(life_stage(c,w.tick)==LifeStage::infant) follow_parent(w,c);
    }
    return advance_clock(w);
}
} // namespace genesis
