#pragma once
// Optional disposable combined geography profile. The caller must explicitly
// generate terrain on new-world creation, then retain that same terrain.
#include "terrain_resources.hpp"
#include "regional_weather.hpp"
#include "livestock_integration.hpp"

namespace genesis::terrain_renewal {
struct Policy {
    const terrain_probe::State& terrain;
    agriculture::Renewal crops;
    const wildlife::Wildlife& wild;
    const regional_weather::Cache* weather=nullptr;
};
// Distribute fractional renewal over 100 pulses with a stable tile phase.
// No extra RNG, float arithmetic, per-tile timer or second map traversal.
// Over a full 100 pulses at constant season, total potential growth is
// exactly base*percent, before stock caps and land-use suppression.
inline unsigned growth(unsigned base,unsigned percent,std::uint64_t tick,std::size_t i) noexcept {
    const auto phase=static_cast<unsigned>((tick/60%100+i%100)%100);
    const auto rate=base*percent;
    return ((phase+1)*rate)/100-(phase*rate)/100;
}
inline void renew_tile(Tile& t,Production& p,std::size_t i,std::uint64_t tick,
                       unsigned food,unsigned water,unsigned wood,const Policy& policy) noexcept {
    const auto& profile=terrain_resources::profiles[static_cast<unsigned>(policy.terrain.cells[i].biome)];
    const auto conditions=policy.weather?
        regional_weather::rates[static_cast<unsigned>(policy.weather->at(tick,i))]:
        regional_weather::rates[0];
    // Natural caps constrain new production, never destroy transferred or
    // harvested stock already above a biome's natural carrying capacity.
    if(policy.crops(i))
        p.food+=renew_stock(t.food,growth(food,regional_weather::modify(profile.food,conditions.food),tick,i),profile.food);
    p.water+=renew_stock(t.water,growth(water,regional_weather::modify(profile.water,conditions.water),tick,i),profile.water);
    // Completed developments stop natural wood renewal. Wild habitat
    // restoration is a later explicit land-use/decay transaction.
    if(!policy.wild.developed[i])
        p.wood+=renew_stock(t.wood,growth(wood,regional_weather::modify(profile.wood,conditions.wood),tick,i),profile.wood);
    // Stone and geological deposits are finite, never renewed here.
}
}
