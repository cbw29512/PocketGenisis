#pragma once
#include "archive_checkpoint.hpp"
#include "ecology.hpp"
#include "replay_lineage.hpp"

namespace genesis::storage {
inline constexpr std::uint64_t archive_replay_limit=3600;
// Single-step reference entry; validate external storage before mutation.
template<class Index> inline bool step_archive_world(World& w,Production& produced,const Index& history) {
    produced={};
    if(!valid_history_view(w,history)) return false;
    const auto validated=[](const World&) noexcept {return true;};
    return genesis::detail::step_world_checked(w,produced,validated);
}
// In a closed replay, new parent/guardian references can only originate from
// living citizens; old external references are fixed at entry. Check all
// resident invariants every tick, with no disk reads inside the tick loop.
template<class Index> inline bool replay_archive(World& w,const Index& history,std::uint64_t ticks) {
    if(!ticks||ticks>archive_replay_limit||ticks>std::numeric_limits<std::uint64_t>::max()-w.tick) return false;
    if(!valid_history_view(w,history)) return false;
    const auto cache=ReplayLineage::build(w,history);
    const auto validate=[&](const World& candidate) noexcept {return cache.valid(candidate);};
    Production produced{};
    for(std::uint64_t i=0;i<ticks;++i)
        if(!genesis::detail::step_world_checked(w,produced,validate)) return false;
    return valid_history_view(w,history);
}
inline bool valid_archive_pair(const ArchiveState& older,const ArchiveState& newer) {
    if(older.generation==std::numeric_limits<std::uint64_t>::max()||
       newer.generation!=older.generation+1||newer.world->seed!=older.world->seed||
       newer.world->tick<older.world->tick) return false;
    if(newer.world->tick==older.world->tick) return valid_retirement_pair(older,newer);
    if(newer.history->count()!=older.history->count()) return false;
    auto replay=std::make_unique<World>(*older.world);
    return replay_archive(*replay,*older.history,newer.world->tick-older.world->tick)&&
           world_payload(*replay)==world_payload(*newer.world);
}
} // namespace genesis::storage
