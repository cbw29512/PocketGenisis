#pragma once
#include "host_archive_checkpoint.hpp"

namespace genesis::storage {
// One bounded, synchronous storage operation per invocation. No wall clock
// dependency or hidden background thread; the eventual device owns cadence.
inline constexpr std::uint64_t cycle_tick_limit=600;
enum class CycleAction { advanced, retired, resumed_retirement };
struct CycleResult {
    ArchiveState state;
    CycleAction action;
    std::size_t retired{};
};
inline CycleResult run_archive_cycle(const std::string& dir,std::uint64_t ticks=cycle_tick_limit) {
    if(!ticks||ticks>cycle_tick_limit) throw std::runtime_error("invalid archive cycle tick budget");
    auto current=load_archive_host(dir);
    if(current.degraded) throw std::runtime_error("degraded checkpoint; automatic cycle refused");
    const auto prefix=current.history->count();
    const auto pending=read_file(history_path(dir,prefix));
    // Resume exactly one already published batch. The retirement transaction
    // compares all bytes against the authoritative resident tail before commit.
    if(!pending.missing) {
        const auto batch=decode_segment(pending.bytes);
        if(batch.seed!=current.world->seed||batch.first!=prefix||
           batch.records.size()>current.world->archive_count)
            throw std::runtime_error("uncommitted segment conflicts with resident history");
        const auto count=batch.records.size();
        return {retire_archive_host(dir,current,count),CycleAction::resumed_retirement,count};
    }
    // Drain a full batch before admitting more ticks. With fewer than 64
    // resident deaths, the existing live+gestation bound leaves ample space
    // for every reserved death and at most one conception in a 600-tick window.
    if(current.world->archive_count>=segment_capacity)
        return {retire_archive_host(dir,current,segment_capacity),CycleAction::retired,segment_capacity};
    return {advance_archive_host(dir,current,ticks),CycleAction::advanced,0};
}
} // namespace genesis::storage
