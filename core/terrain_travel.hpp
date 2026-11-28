#pragma once
#include "terrain_navigation.hpp"
namespace genesis::navigation {
// Per-citizen travel state. Future integrated checkpoint must serialize this
// alongside citizen identity; do not silently load it into old PGCI saves.
struct TravelProgress {
    std::uint16_t from{unreachable},to{unreachable};
    unsigned remaining{};
    std::uint64_t last_tick{};
    bool attempted{};
    void cancel() noexcept {from=to=unreachable;remaining=0;}
};
enum class TravelResult { blocked, waiting, moved, already_used };
// One movement action per citizen per authoritative tick. A difficult edge
// consumes multiple actions. Survival can interrupt without teleporting.
// The caller supplies the intended adjacent tile from the shared router.
inline TravelResult advance(const Terrain& terrain,Citizen& citizen,
                            TravelProgress& progress,std::size_t next,
                            std::uint64_t tick) noexcept {
    if(progress.attempted&&tick<=progress.last_tick)return TravelResult::already_used;
    progress.attempted=true;progress.last_tick=tick;
    if(citizen.x>=width||citizen.y>=height){progress.cancel();return TravelResult::blocked;}
    const auto from=std::size_t(citizen.y)*width+citizen.x;
    const auto required=cost(terrain,from,next);
    if(!required){progress.cancel();return TravelResult::blocked;}
    if(progress.from!=from||progress.to!=next||!progress.remaining||progress.remaining>required){
        progress.from=static_cast<std::uint16_t>(from);
        progress.to=static_cast<std::uint16_t>(next);progress.remaining=required;
    }
    citizen.action=Action::move;
    if(--progress.remaining)return TravelResult::waiting;
    citizen.x=static_cast<std::uint16_t>(next%width);
    citizen.y=static_cast<std::uint16_t>(next/width);
    progress.cancel();return TravelResult::moved;
}
}
