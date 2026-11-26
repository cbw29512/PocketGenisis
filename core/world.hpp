#pragma once
#include "rng.hpp"
#include <array>
#include <limits>

namespace genesis {
inline constexpr std::uint16_t width = 100, height = 60;
inline constexpr std::uint64_t ticks_per_year = 43200; // 1 tick/real second
// Stable numeric stream order is part of the versioned replay contract.
enum Stream : std::size_t { terrain, traits, environment, decisions, reproduction, research, stream_count };
struct Tile {
    std::uint16_t food{}, wood{}, water{}, stone{};
};
enum class Action : std::uint8_t { idle, move, eat, drink };
struct Founder {
    std::uint64_t id{};
    std::uint16_t x{}, y{};
    std::array<std::uint8_t, 6> personality{};
    std::uint8_t hunger{}, thirst{};
    Action action{Action::idle};
    // Founder type is intrinsically immortal; mortal agents are a later gate.
};
struct World {
    std::uint64_t seed{}, tick{};
    std::array<Rng, stream_count> streams{};
    std::array<Tile, width * height> tiles{};
    std::array<Founder, 2> founders{};
};
// Call only for explicit creation of a disposable world. Never on load failure.
inline void initialize(World& w, std::uint64_t seed) noexcept {
    w.seed = seed;
    w.tick = 0;
    Rng root{seed};
    for (auto& s : w.streams) s.state = root.next();
    auto& map = w.streams[terrain];
    for (auto& t : w.tiles) {
        std::uint32_t v{};
        map.bounded(41, v); t.food = static_cast<std::uint16_t>(60 + v);
        map.bounded(41, v); t.wood = static_cast<std::uint16_t>(60 + v);
        map.bounded(41, v); t.water = static_cast<std::uint16_t>(60 + v);
        map.bounded(41, v); t.stone = static_cast<std::uint16_t>(20 + v);
    }
    for (std::size_t i = 0; i < w.founders.size(); ++i) {
        auto& f = w.founders[i];
        f = Founder{}; // Clear transient state on explicit disposable initialization.
        f.id = i + 1;
        f.x = static_cast<std::uint16_t>(width / 2 + i);
        f.y = height / 2;
        for (auto& trait : f.personality) {
            std::uint32_t v{};
            w.streams[traits].bounded(101, v);
            trait = static_cast<std::uint8_t>(v);
        }
    }
}
// Clock only in this increment. No weather, survival or action transitions yet.
inline bool advance_clock(World& w) noexcept {
    if (w.tick == std::numeric_limits<std::uint64_t>::max()) return false;
    ++w.tick;
    return true;
}
inline unsigned season(const World& w) noexcept {
    return static_cast<unsigned>((w.tick % ticks_per_year) / (ticks_per_year / 4));
}
} // namespace genesis
