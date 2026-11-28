#pragma once
#include "livestock.hpp"

namespace genesis::livestock {
// Check the same local transaction at each of at most five candidate tiles.
// Movement is ordinary, one tile at a time; no remote animal or food transfer.
inline bool seek_work(const World& w,const discovery::State& knowledge,
                      const construction::State& sites,const wildlife::Wildlife& wild,
                      const State& s,Citizen& c) noexcept {
    if(!ready_pen(w,sites)||!is_founder(c)||!discovery::available(c)||
       c.hunger>8||c.thirst>8||!(knowledge.known[c.id-1]&discovery::tech_bit(discovery::husbandry))) return false;
    const auto pen=sites.sites[construction::enclosure].tile;
    const int px=pen%width,py=pen/width;
    constexpr int dx[]={0,-1,1,0,0},dy[]={0,0,0,-1,1};
    int best=settlement::home_radius+1;unsigned priority=4;std::uint16_t target=0;
    for(unsigned i=0;i<5;++i) {
        const int x=px+dx[i],y=py+dy[i];
        if(x<0||x>=width||y<0||y>=height) continue;
        const int d=std::abs(int(c.x)-x)+std::abs(int(c.y)-y);
        if(!d||d>settlement::home_radius) continue;
        Citizen probe=c;probe.x=static_cast<std::uint16_t>(x);probe.y=static_cast<std::uint16_t>(y);
        const auto task=choose(w,knowledge,sites,wild,s,probe).task;
        if(task==Task::none) continue;
        // Capture/harvest before supplies, then shortest distance and stable tile.
        const unsigned rank=task==Task::capture?0:task==Task::harvest?1:2;
        const auto tile=construction::location(probe);
        if(rank<priority||(rank==priority&&(d<best||(d==best&&tile<target)))) {
            priority=rank;best=d;target=tile;
        }
    }
    if(priority==4) return false;
    move_toward(c,target%width,target/width);return true;
}
}
