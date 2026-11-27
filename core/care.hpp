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
using CareRequests=std::array<std::uint16_t,live_capacity>;
// Temporary local care, never a change to biological genealogy.
inline const Citizen* local_orphan_carer(const World& w,const Citizen& child,
                                        int radius,const CareRequests* requests=nullptr) noexcept {
    auto eligible=[&](const Citizen& c) {
        return life_stage(c,w.tick)>=LifeStage::adult&&c.health>=50&&c.personality[2]>=50&&
               c.hunger<11&&c.thirst<11&&(!requests||!(*requests)[live_slot(w,c.id)]);
    };
    if(const auto* guardian=live_citizen(w,child.guardian_id)) {
        if(eligible(*guardian)&&distance_between(*guardian,child)<=radius) return guardian;
    }
    const Citizen* best=nullptr;
    int best_distance=radius+1;
    auto consider=[&](const Citizen& c) {
        if(!eligible(c)) return;
        const int distance=distance_between(c,child);
        if(distance>radius) return;
        // Nearest available adult; stable compassion and ID tie breakers.
        if(!best||distance<best_distance||(distance==best_distance&&
           (c.personality[2]>best->personality[2]||
            (c.personality[2]==best->personality[2]&&c.id<best->id)))) {
            best=&c;best_distance=distance;
        }
    };
    for(const auto& c:w.founders) consider(c);
    for(std::size_t i=0;i<w.descendant_count;++i) consider(w.descendants[i]);
    return best;
}
inline CareRequests prepare_care(World& w) noexcept {
    CareRequests requests{};
    for(std::size_t i=0;i<w.descendant_count;++i) {
        auto& c=w.descendants[i];
        if(life_stage(c,w.tick)!=LifeStage::infant) continue;
        accrue_needs(c);
        if(!urgent_need(c)) continue;
        for(auto id:{c.parent1,c.parent2}) {
            const auto* p=live_citizen(w,id);
            const auto slot=live_slot(w,id);
            if(p&&distance_between(*p,c)<=1&&!requests[slot])
                requests[slot]=static_cast<std::uint16_t>(i+1);
        }
    }
    // Biological dependents reserve their parents first. Orphans compete only
    // for remaining local adult actions, in ascending infant ID order.
    for(std::size_t i=0;i<w.descendant_count;++i) {
        const auto& c=w.descendants[i];
        if(life_stage(c,w.tick)!=LifeStage::infant||!urgent_need(c)||
           live_citizen(w,c.parent1)||live_citizen(w,c.parent2)) continue;
        if(const auto* carer=local_orphan_carer(w,c,1,&requests))
            requests[live_slot(w,carer->id)]=static_cast<std::uint16_t>(i+1);
    }
    return requests;
}
inline bool give_care(World& w,Citizen& parent,Citizen* child) noexcept {
    if(!child||child->action!=Action::idle||!urgent_need(*child)||
       urgent_need(parent)||parent.health<50||distance_between(parent,*child)>1) return false;
    if(!consume_need(w,*child,parent.x,parent.y)) return false;
    parent.action=Action::care;
    // Only an actual successful care transaction establishes guardianship.
    if(life_stage(*child,w.tick)==LifeStage::infant&&
       !live_citizen(w,child->parent1)&&!live_citizen(w,child->parent2)&&
       !live_citizen(w,child->guardian_id)) {
        child->guardian_id=parent.id;
        child->guardian_since_tick=w.tick;
    }
    return true;
}
inline void follow_parent(World& w,Citizen& child) noexcept {
    if(child.action!=Action::idle) return;
    const auto* a=live_citizen(w,child.parent1);
    const auto* b=live_citizen(w,child.parent2);
    const auto* p=!a?b:!b?a:distance_between(child,*a)<=distance_between(child,*b)?a:b;
    // Follow a known local guardian even when they cannot offer care now.
    if(!p) {
        const auto* guardian=live_citizen(w,child.guardian_id);
        p=guardian&&distance_between(child,*guardian)<=4?guardian:local_orphan_carer(w,child,4);
    }
    if(!p||!distance_between(child,*p)) return;
    // Dependent transport abstraction; no RNG or independent resource search.
    if(child.x!=p->x) child.x=static_cast<std::uint16_t>(int(child.x)+(child.x<p->x?1:-1));
    else child.y=static_cast<std::uint16_t>(int(child.y)+(child.y<p->y?1:-1));
    child.action=Action::move;
}
} // namespace genesis
