#pragma once
#include "rng.hpp"
#include <array>
#include <limits>

namespace genesis {
inline constexpr std::uint16_t width = 100, height = 60;
inline constexpr std::uint64_t ticks_per_year = 43200; // 1 tick/real second
// Stable numeric stream order is part of the versioned replay contract.
enum Stream : std::size_t { terrain, traits, environment, decisions, reproduction, research, health_events, immunity_traits, disease_events, parental_health_events, stream_count };
struct Tile {
    std::uint16_t food{}, wood{}, water{}, stone{};
};
enum class Action : std::uint8_t { idle, move, eat, drink, care };
enum class Purpose : std::uint8_t { none, gatherer, builder, inventor };
enum class LifeStage : std::uint8_t { infant, child, adolescent, adult, elder };
enum class BirthOutcome : std::uint8_t { normal, complicated, fatal };
inline constexpr std::size_t disease_count=2;
struct DiseaseState {
    std::uint16_t illness{}, immunity{};
    std::uint8_t resistance{50};
};
struct Citizen {
    std::uint64_t id{};
    std::uint16_t x{}, y{};
    std::array<std::uint8_t, 6> personality{};
    std::uint8_t hunger{}, thirst{};
    Action action{Action::idle};
    std::uint64_t parent1{}, parent2{}, birth_tick{};
    std::uint64_t guardian_id{}, guardian_since_tick{};
    Purpose purpose{Purpose::none};
    std::uint8_t health{100}, lifespan_years{70};
    BirthOutcome birth_outcome{BirthOutcome::normal};
    std::array<DiseaseState,disease_count> diseases{};
};
using Founder = Citizen;
// Alternate capacities are only for isolated, disposable CPU benchmark builds.
// Do not link differently configured World definitions into one executable.
#ifdef PG_DISPOSABLE_SCALE_PROBE
inline constexpr std::size_t descendant_capacity=2048;
inline constexpr std::size_t history_capacity=4096;
#else
inline constexpr std::size_t descendant_capacity=254;
inline constexpr std::size_t history_capacity=512;
#endif
// The legacy identity index reserves its high bit for archived records.
static_assert(history_capacity<0x8000 && descendant_capacity<=history_capacity);
enum class DeathCause : std::uint8_t { starvation=1, dehydration=2, old_age=3, childbirth=4, disease=5, parental_childbirth=6 };
struct DeathRecord {
    Citizen citizen{};
    std::uint64_t death_tick{};
    DeathCause cause{DeathCause::old_age};
};
inline constexpr std::uint64_t gestation_ticks=16200;
inline Purpose choose_purpose(const Citizen& c) noexcept {
    // Fixed tie ordering makes assignment reproducible.
    if(c.personality[0]>=c.personality[1] && c.personality[0]>=c.personality[2])
        return Purpose::inventor;
    return c.personality[1]>=c.personality[2]?Purpose::builder:Purpose::gatherer;
}
inline LifeStage life_stage(const Citizen& c,std::uint64_t tick) noexcept {
    if(c.id==1 || c.id==2) return LifeStage::adult;
    const auto age=(tick>=c.birth_tick?tick-c.birth_tick:0)/ticks_per_year;
    if(age<2) return LifeStage::infant;
    if(age<12) return LifeStage::child;
    if(age<18) return LifeStage::adolescent;
    return age<55?LifeStage::adult:LifeStage::elder;
}
struct Pregnancy {
    std::uint64_t parent1{}, parent2{}, due_tick{};
};
inline constexpr std::size_t pregnancy_capacity=(descendant_capacity+2)/2;
struct World {
    std::uint64_t seed{}, tick{};
    std::array<Rng, stream_count> streams{};
    std::array<Tile, width * height> tiles{};
    std::array<Founder, 2> founders{};
    std::array<Citizen, descendant_capacity> descendants{};
    std::size_t descendant_count{};
    std::array<DeathRecord, history_capacity> archive{};
    std::size_t archive_count{};
    std::uint64_t next_citizen_id{3}, lost_gestations{};
    std::array<Pregnancy,pregnancy_capacity> pregnancies{};
    std::size_t pregnancy_count{};
};
// Call only for explicit creation of a disposable world. Never on load failure.
inline void initialize(World& w, std::uint64_t seed) noexcept {
    w.seed = seed;
    w.tick = 0;
    w.descendant_count=0;
    w.archive_count=0;
    for(auto& d:w.archive) d=DeathRecord{};
    w.next_citizen_id=3;
    w.pregnancies.fill(Pregnancy{});
    w.pregnancy_count=0;
    w.lost_gestations=0;
    for(auto& c:w.descendants) c=Citizen{};
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
        f.purpose=choose_purpose(f);
        for(auto& d:f.diseases) {
            std::uint32_t v{};w.streams[immunity_traits].bounded(101,v);
            d.resistance=static_cast<std::uint8_t>(v);
        }
    }
}
// Low-level clock primitive. Use step_world for complete simulation ticks.
inline bool advance_clock(World& w) noexcept {
    if (w.tick == std::numeric_limits<std::uint64_t>::max()) return false;
    ++w.tick;
    return true;
}
inline unsigned season(const World& w) noexcept {
    return static_cast<unsigned>((w.tick % ticks_per_year) / (ticks_per_year / 4));
}
} // namespace genesis
