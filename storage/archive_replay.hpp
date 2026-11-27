#pragma once
#include "archive_checkpoint.hpp"
#include "ecology.hpp"

namespace genesis::storage {
// Host reference: full external lineage validation each tick, not device path.
inline constexpr std::uint64_t archive_replay_limit=3600;
inline bool step_archive_world(World& w,Production& produced,const HistoryIndex& history) noexcept {
    const auto validate=[&](const World& candidate) noexcept {return valid_history_view(candidate,history);};
    return genesis::detail::step_world_checked(w,produced,validate);
}
inline bool replay_archive(World& w,const HistoryIndex& history,std::uint64_t ticks) noexcept {
    if(!ticks||ticks>archive_replay_limit||ticks>std::numeric_limits<std::uint64_t>::max()-w.tick) return false;
    Production produced{};
    for(std::uint64_t i=0;i<ticks;++i) if(!step_archive_world(w,produced,history)) return false;
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
