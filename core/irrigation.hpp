#pragma once
#ifndef PG_IRRIGATION_PROBE
#error "Irrigation is an isolated probe profile"
#endif
#include "agriculture.hpp"

namespace genesis::irrigation {
inline constexpr unsigned rate=2;
inline constexpr std::uint64_t interval=60;
struct State {std::uint64_t last_transfer{},transfers{},water{};};
inline bool valid(const State& s) noexcept {
    return s.water>=s.transfers&&s.water<=s.transfers*rate&&
           (s.transfers==0)==(s.last_transfer==0);
}
inline unsigned transfer(World& w,const construction::State& sites,State& s) noexcept {
    if(!w.tick||w.tick%interval||w.tick<=s.last_transfer) return 0;
    const auto& channel=sites.sites[construction::channel];
    const auto& field=sites.sites[construction::field];
    if(channel.stage!=construction::Stage::complete||field.stage!=construction::Stage::complete||
       channel.tile>=w.tiles.size()||field.tile>=w.tiles.size()||
       !construction::channel_adjacent(channel.tile,field.tile)) return 0;
    auto& source=w.tiles[channel.tile].water;
    auto& dest=w.tiles[field.tile].water;
    if(source<=construction::irrigation_source_reserve||dest>=construction::irrigation_target) return 0;
    const auto n=std::min({rate,unsigned(source-construction::irrigation_source_reserve),
                         unsigned(construction::irrigation_target-dest)});
    source-=n;dest+=n; // Equal debit and credit, never new production.
    s.last_transfer=w.tick;++s.transfers;s.water+=n;
    return n;
}
inline bool step(World& w,discovery::State& research,construction::State& sites,
                 storage::State& reserve,cooking::State& cooked,shelter::State& sheltered,
                 settlement::State& home,agriculture::State& crops,State& water,
                 Production& p,discovery::Event& re,construction::Event& ce,std::uint64_t cadence) noexcept {
    if(!agriculture::step(w,research,sites,reserve,cooked,sheltered,home,crops,p,re,ce,cadence)) return false;
    transfer(w,sites,water); // Water already in the world: Production unchanged.
    return true;
}
}
