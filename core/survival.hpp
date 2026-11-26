#pragma once
#include "world.hpp"
#include "resource_search.hpp"
#include <algorithm>

namespace genesis {
inline bool valid_agent(const Citizen& c) noexcept {
    if(c.x>=width || c.y>=height || c.hunger>100 || c.thirst>100 || c.health>100 ||
       static_cast<unsigned>(c.action)>static_cast<unsigned>(Action::drink)) return false;
    for(auto trait:c.personality) if(trait>100) return false;
    return static_cast<unsigned>(c.purpose)<=static_cast<unsigned>(Purpose::inventor);
}
// Validate live/archive identity conservation before mutating the world.
inline bool valid_population(const World& w) noexcept {
    if(w.descendant_count>descendant_capacity || w.archive_count>history_capacity ||
       w.descendant_count+w.archive_count>history_capacity ||
       w.next_citizen_id!=w.descendant_count+w.archive_count+3) return false;
    std::array<bool,history_capacity> seen{};
    auto identity=[&](const Citizen& c) {
        if(c.id<3 || c.id>=w.next_citizen_id || c.parent1!=1 || c.parent2!=2 ||
           c.birth_tick==0 || c.birth_tick>w.tick || c.lifespan_years<60 ||
           c.lifespan_years>80 || !valid_agent(c)) return false;
        auto& used=seen[static_cast<std::size_t>(c.id-3)];
        if(used) return false;
        used=true;
        return true;
    };
    for(std::size_t i=0;i<w.founders.size();++i) {
        const auto& c=w.founders[i];
        if(!valid_agent(c)||c.id!=i+1||c.parent1||c.parent2||c.birth_tick||
           c.purpose==Purpose::none||c.health==0) return false;
    }
    std::uint64_t last_id=2;
    for(std::size_t i=0;i<w.descendant_count;++i) {
        const auto& c=w.descendants[i];
        if(!identity(c)||c.id<=last_id||c.health==0) return false;
        if(life_stage(c,w.tick)<LifeStage::adult && c.purpose!=Purpose::none) return false;
        last_id=c.id;
    }
    for(std::size_t i=0;i<w.archive_count;++i) {
        const auto& d=w.archive[i]; const auto& c=d.citizen;
        if(!identity(c)||d.death_tick<c.birth_tick||d.death_tick>w.tick||
           static_cast<unsigned>(d.cause)<1||static_cast<unsigned>(d.cause)>3||
           (d.cause==DeathCause::old_age?c.health==0:c.health!=0)) return false;
        if(i) {
            const auto& previous=w.archive[i-1];
            if(d.death_tick<previous.death_tick ||
               (d.death_tick==previous.death_tick&&c.id<=previous.citizen.id)) return false;
        }
    }
    return !w.founder_due_tick ||
        (w.founder_due_tick>w.tick && w.founder_due_tick-w.tick<=gestation_ticks &&
         w.descendant_count<descendant_capacity &&
         w.descendant_count+w.archive_count<history_capacity);
}
// Shared authoritative survival transaction for every active citizen.
inline void act_survival(World& w,Citizen& f) noexcept {
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
        return;
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
inline bool step_survival(World& w) noexcept {
    if(w.tick==std::numeric_limits<std::uint64_t>::max() || !valid_population(w)) return false;
    for(auto& f:w.founders) act_survival(w,f);
    for(std::size_t i=0;i<w.descendant_count;++i) act_survival(w,w.descendants[i]);
    return advance_clock(w);
}
} // namespace genesis
