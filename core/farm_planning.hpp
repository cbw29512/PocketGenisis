#pragma once
#include "construction_work.hpp"

namespace genesis::farm_planning {
// A viable project target is a planning hint only; final admission still
// checks real location, materials, knowledge, need and unoccupied land.
inline bool seek(World const& w,const discovery::State& research,
                 const construction::State& sites,Citizen& c) noexcept {
    if(!is_founder(c)||!discovery::available(c)||c.hunger>8||c.thirst>8) return false;
    const auto owner=unsigned(c.id-1);
    const auto active=construction::active(sites);
    if(active>=0) {
        const auto& job=sites.sites[unsigned(active)];
        if(job.owner!=owner||job.tile>=w.tiles.size()||job.tile==construction::location(c)) return false;
        move_toward(c,job.tile%width,job.tile/width);return true;
    }
    if(research.job.active) return false;
    // Only search for a needed first farm (or extra farm in a larger family).
    // Bounded scan around the existing settlement, not the entire world.
    if(!(research.known[owner]&discovery::tech_bit(discovery::farming))) return false;
    int wanted=-1;unsigned established=0;
    for(unsigned i=0;i<construction::field_count;++i) {
        const auto slot=construction::field_slot(i);
        if(sites.sites[slot].stage==construction::Stage::complete) ++established;
        if(wanted<0&&sites.sites[slot].stage==construction::Stage::empty) wanted=int(slot);
    }
    if(wanted<0||(established&&w.descendant_count+2<=4*established)) return false;
    int best=width+height;std::uint16_t target{};bool found=false;
    // Every candidate is within radius 8 of a completed hut. Stable row-major
    // tile ties avoid new random streams; no temporary map or heap allocation.
    for(unsigned k=0;k<construction::site_count;++k) {
        const auto& hut=sites.sites[k];
        if(construction::kind_at(k)!=construction::hut||
           hut.stage!=construction::Stage::complete||hut.tile>=w.tiles.size()) continue;
        const int hx=hut.tile%width,hy=hut.tile/width;
        for(int y=std::max(0,hy-8);y<=std::min(int(height)-1,hy+8);++y) {
            const int radius=8-std::abs(y-hy);
            for(int x=std::max(0,hx-radius);x<=std::min(int(width)-1,hx+radius);++x) {
                const int distance=std::abs(int(c.x)-x)+std::abs(int(c.y)-y);
                const auto tile=static_cast<std::uint16_t>(y*width+x);
                if(distance>best||(distance==best&&found&&tile>=target)) continue;
                Citizen candidate=c;candidate.x=static_cast<std::uint16_t>(x);
                candidate.y=static_cast<std::uint16_t>(y);
                if(!construction::can_start_at(w,research,sites,owner,unsigned(wanted),candidate)) continue;
                best=distance;target=tile;found=true;
            }
        }
    }
    if(!found||!best) return false;
    move_toward(c,target%width,target/width);return true;
}
}
