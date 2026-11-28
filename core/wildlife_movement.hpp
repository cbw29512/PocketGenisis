#pragma once
// Included by wildlife.hpp after the state and transaction declarations.

namespace genesis::wildlife {
// Local food score: vegetation for grazers, living prey for predators.
// Subtract existing competition; the moving group is excluded at its origin.
inline unsigned support(const World& w,const Wildlife& state,std::size_t i,
                        std::size_t k,bool origin,bool prey_avoidance=false) noexcept {
    if(state.developed[i]) return 0;
    const auto& t=state.tiles[i];const auto& resource=w.tiles[i];
    unsigned grazers=0;
    for(std::size_t j=0;j<wolf;++j) grazers+=t.count[j];
    unsigned food=k<wolf?unsigned(resource.food):grazers;
    if(k==bear) food+=resource.food;
    unsigned demand=k<wolf?grazers:unsigned(t.count[wolf])+t.count[bear];
    if(origin) demand-=t.count[k];
    // Local danger is a finite cost, not an impassable wall or a forced escape.
    // Keep resource availability essential; predators still follow living prey.
    if(prey_avoidance&&k<wolf) demand+=2u*(unsigned(t.count[wolf])+t.count[bear]);
    const auto available=std::min(food,unsigned(resource.water));
    return available>demand?available-demand:0;
}
// One checkerboard color can move to the other per pulse. This ensures each
// group moves at most one tile without an additional map-sized scratch array.
// Developed origins are always considered; evacuations run after normal
// movement so arrivals are never moved a second time in the same pulse.
inline void migrate(const World& w,Wildlife& state,Totals& out,bool prey_avoidance=false) noexcept {
    const auto color=(w.tick/60)%2;
    for(unsigned pass=0;pass<2;++pass) {
        for(std::size_t i=0;i<state.tiles.size();++i) {
            const auto x=i%width,y=i/width;
            const bool built=state.developed[i]!=0;
            if(pass==0?(built||(x+y)%2!=color):!built) continue;
            auto& from=state.tiles[i];
            for(std::size_t k=0;k<species_count;++k) {
                if(!from.count[k]) continue;
                auto best=i;auto score=support(w,state,i,k,true,prey_avoidance);
                const auto consider=[&](std::size_t j) noexcept {
                    if(state.developed[j]||state.tiles[j].count[k]>=group_limit) return;
                    const auto candidate=support(w,state,j,k,false,prey_avoidance);
                    if(candidate>score||(built&&best==i)) {best=j;score=candidate;}
                };
                if(x) consider(i-1);
                if(x+1<width) consider(i+1);
                if(y) consider(i-width);
                if(y+1<height) consider(i+width);
                if(best==i) continue;
                auto& dest=state.tiles[best].count[k];
                const auto room=group_limit-unsigned(dest);
                // Evacuate even if the destination is food-poor. Feeding
                // and mortality resolve after movement, not during transfer.
                const auto moved=std::min({unsigned(from.count[k]),room,built?room:score});
                from.count[k]-=moved;dest+=moved;out.migrated+=moved;
            }
        }
    }
}
} // namespace genesis::wildlife
