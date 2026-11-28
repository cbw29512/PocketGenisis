#pragma once
#include "world.hpp"
#include "byte_codec.hpp"
namespace genesis::storage {
// One field order for writing and reading; changing it requires a version bump.
template<bool Drives=false,class IO,class Person> void citizen_fields(IO& io,Person& c) {
    io.field(c.id);io.field(c.x);io.field(c.y);
    for(auto& trait:c.personality) io.field(trait);
    io.field(c.hunger);io.field(c.thirst);
    if constexpr(Drives) {
#ifdef PG_FATIGUE_PROBE
        io.field(c.fatigue);io.field(c.rest_intent);
#endif
#ifdef PG_REPRODUCTIVE_DRIVE_PROBE
        io.field(c.reproductive_drive);
#endif
    }
    io.field(c.action);
    io.field(c.parent1);io.field(c.parent2);io.field(c.birth_tick);
    io.field(c.guardian_id);io.field(c.guardian_since_tick);
    io.field(c.purpose);io.field(c.health);io.field(c.lifespan_years);io.field(c.birth_outcome);
    for(auto& d:c.diseases) {io.field(d.illness);io.field(d.immunity);io.field(d.resistance);}
}
template<bool Drives=false,class IO,class W> void world_fields(IO& io,W& w) {
    io.field(w.seed);io.field(w.tick);io.field(w.next_citizen_id);io.field(w.lost_gestations);
    for(auto& r:w.streams) io.field(r.state);
    for(auto& t:w.tiles) {io.field(t.food);io.field(t.wood);io.field(t.water);io.field(t.stone);}
    for(auto& f:w.founders) citizen_fields<Drives>(io,f);
    io.count(w.descendant_count,descendant_capacity);
    for(std::size_t i=0;i<w.descendant_count;++i) citizen_fields<Drives>(io,w.descendants[i]);
    io.count(w.archive_count,history_capacity);
    for(std::size_t i=0;i<w.archive_count;++i) {
        auto& d=w.archive[i];citizen_fields<Drives>(io,d.citizen);io.field(d.death_tick);io.field(d.cause);
    }
    io.count(w.pregnancy_count,pregnancy_capacity);
    for(std::size_t i=0;i<w.pregnancy_count;++i) {
        auto& p=w.pregnancies[i];io.field(p.parent1);io.field(p.parent2);io.field(p.due_tick);
    }
}
} // namespace genesis::storage
