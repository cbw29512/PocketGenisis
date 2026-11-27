#pragma once
#include "population.hpp"
#include <algorithm>
#include <cstdlib>

namespace genesis {
inline void accrue_needs(Citizen& c) noexcept {
    c.action=Action::idle;
    c.hunger=static_cast<std::uint8_t>(std::min(100,c.hunger+1));
    c.thirst=static_cast<std::uint8_t>(std::min(100,c.thirst+1));
}
inline bool urgent_need(const Citizen& c) noexcept {
    return c.hunger>=12||c.thirst>=12;
}
inline int distance_between(const Citizen& a,const Citizen& b) noexcept {
    return std::abs(int(a.x)-int(b.x))+std::abs(int(a.y)-int(b.y));
}
inline std::uint16_t& resource_stock(World& w,int x,int y,bool water) noexcept {
    auto& t=w.tiles[static_cast<std::size_t>(y)*width+static_cast<std::size_t>(x)];
    return water?t.water:t.food;
}
// Authoritative consumption, shared by independent survival and dependent care.
inline bool consume_resource(World& w,Citizen& recipient,int x,int y,bool water) noexcept {
    auto& stock=resource_stock(w,x,y,water);
    if(!stock) return false;
    --stock;
    auto& need=water?recipient.thirst:recipient.hunger;
    need=static_cast<std::uint8_t>(std::max(0,need-12));
    recipient.action=water?Action::drink:Action::eat;
    return true;
}
inline bool consume_need(World& w,Citizen& recipient,int x,int y) noexcept {
    return consume_resource(w,recipient,x,y,recipient.thirst>=recipient.hunger);
}
using CareRequests=std::array<std::uint16_t,history_capacity+3>;
inline CareRequests prepare_care(World& w) noexcept {
    CareRequests requests{};
    for(std::size_t i=0;i<w.descendant_count;++i) {
        auto& c=w.descendants[i];
        if(life_stage(c,w.tick)!=LifeStage::infant) continue;
        accrue_needs(c);
        if(!urgent_need(c)) continue;
        for(auto id:{c.parent1,c.parent2}) {
            const auto* p=live_citizen(w,id);
            if(p&&distance_between(*p,c)<=1&&!requests[static_cast<std::size_t>(id)])
                requests[static_cast<std::size_t>(id)]=static_cast<std::uint16_t>(i+1);
        }
    }
    return requests;
}
inline bool give_care(World& w,Citizen& parent,Citizen* child) noexcept {
    if(!child||child->action!=Action::idle||!urgent_need(*child)||
       urgent_need(parent)||parent.health<50||distance_between(parent,*child)>1) return false;
    if(!consume_need(w,*child,parent.x,parent.y)) return false;
    parent.action=Action::care;
    return true;
}
inline void follow_parent(World& w,Citizen& child) noexcept {
    if(child.action!=Action::idle) return;
    const auto* a=live_citizen(w,child.parent1);
    const auto* b=live_citizen(w,child.parent2);
    const auto* p=!a?b:!b?a:distance_between(child,*a)<=distance_between(child,*b)?a:b;
    if(!p||!distance_between(child,*p)) return;
    // Dependent transport abstraction; no RNG or independent resource search.
    if(child.x!=p->x) child.x=static_cast<std::uint16_t>(int(child.x)+(child.x<p->x?1:-1));
    else child.y=static_cast<std::uint16_t>(int(child.y)+(child.y<p->y?1:-1));
    child.action=Action::move;
}
} // namespace genesis
