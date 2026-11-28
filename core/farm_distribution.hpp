#pragma once
#include "construction.hpp"
#include "resource_transfer.hpp"

namespace genesis::farm_distribution {
inline constexpr unsigned reserve=24,target=80,batch=4;
// Choose a real adjacent destination with demand. Completed fields are
// excluded: moving surplus between full plots cannot create storage.
inline int destination(const World& w,const construction::State& sites,unsigned tile) noexcept {
    if(tile>=w.tiles.size()) return -1;
    const int x=tile%width,y=tile/width;
    constexpr int dx[]={-1,1,0,0},dy[]={0,0,-1,1};
    int best=-1;unsigned lowest=target;
    for(unsigned i=0;i<4;++i) {
        const int nx=x+dx[i],ny=y+dy[i];
        if(nx<0||nx>=width||ny<0||ny>=height) continue;
        const auto index=unsigned(ny*width+nx);
        bool field=false;
        for(unsigned j=0;j<construction::field_count;++j) {
            const auto& plot=sites.sites[construction::field_slot(j)];
            if(plot.stage==construction::Stage::complete&&plot.tile==index) field=true;
        }
        if(!field&&w.tiles[index].food<lowest) {
            best=int(index);lowest=w.tiles[index].food;
        }
    }
    return best;
}
inline unsigned deliver(World& w,const construction::State& sites,unsigned tile) noexcept {
    const auto dest=destination(w,sites,tile);
    if(dest<0) return 0;
    return resource_transfer::move(w.tiles[tile].food,w.tiles[unsigned(dest)].food,reserve,target,batch);
}
}
