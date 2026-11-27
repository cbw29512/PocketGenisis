#pragma once
#ifndef PG_CROP_LOSS_PROBE
#error "Crop loss requires its isolated probe profile"
#endif
#include "multi_field.hpp"

namespace genesis::crop_loss {
inline constexpr std::uint64_t drought_limit=600,ripe_limit=3600;
enum class Cause : std::uint8_t { none,drought,spoilage };
// Post-tick evaluation: existing farming and irrigation have already run.
// A crop loss destroys standing crop only, not food already in tile stock.
inline Cause evaluate(const World& w,const construction::State& sites,
                      agriculture::State& crop,unsigned slot) noexcept {
    if(!w.tick||slot>=construction::site_count||construction::kind_at(slot)!=construction::field)
        return Cause::none;
    const auto& plot=sites.sites[slot];
    if(!crop.stage||crop.stage>4||plot.stage!=construction::Stage::complete||
       plot.tile>=w.tiles.size()) {crop.dry_since=0;return Cause::none;}
    Cause reason=Cause::none;
    if(crop.stage==4) {
        crop.dry_since=0;
        // At stage four the crop has received its final tending, not yet harvested.
        // Give it the ordinary growing interval plus the full ripe grace interval.
        if(w.tick>=crop.last_work&&w.tick-crop.last_work>=agriculture::work_interval+ripe_limit)
            reason=Cause::spoilage;
    } else {
        const bool winter=(w.tick%ticks_per_year)/(ticks_per_year/4)==3;
        if(winter||w.tiles[plot.tile].water) crop.dry_since=0;
        else {
            if(!crop.dry_since||crop.dry_since>w.tick) crop.dry_since=w.tick;
            if(w.tick-crop.dry_since>=drought_limit) reason=Cause::drought;
        }
    }
    if(reason==Cause::none) return reason;
    crop.lost_water+=crop.stage; // Water and seed previously spent are not refunded.
    if(reason==Cause::drought) ++crop.drought_losses;
    else ++crop.spoilage_losses;
    crop.stage=0;crop.dry_since=0;crop.last_work=w.tick;
    return reason;
}
inline bool step(World& w,discovery::State& research,construction::State& sites,
                 storage::State& reserve,cooking::State& cooked,shelter::State& sheltered,
                 settlement::State& home,multi_field::State& farms,irrigation::State& water,
                 Production& p,discovery::Event& re,construction::Event& ce,std::uint64_t cadence) noexcept {
    if(!multi_field::step(w,research,sites,reserve,cooked,sheltered,home,farms,water,p,re,ce,cadence)) return false;
    for(unsigned i=0;i<construction::field_count;++i)
        evaluate(w,sites,farms.crops[i],construction::field_slot(i));
    return true;
}
}
