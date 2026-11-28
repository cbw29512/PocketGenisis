#pragma once
#include "world_codec_fields.hpp"
namespace genesis::storage {
inline void flag(Writer& io,bool v) {io.field(static_cast<std::uint8_t>(v));}
inline void flag(Reader& io,bool& v) {
    std::uint8_t n{};io.field(n);
    if(n>1) throw std::runtime_error("invalid boolean");
    v=n!=0;
}
inline void tile_index(Writer& io,std::size_t v) {
    if(v>std::size_t(width)*height) throw std::runtime_error("invalid tile index");
    io.field(static_cast<std::uint16_t>(v));
}
inline void tile_index(Reader& io,std::size_t& v) {
    std::uint16_t n{};io.field(n);
    if(n>std::size_t(width)*height) throw std::runtime_error("invalid tile index");
    v=n;
}
// Explicit field order: never serialize raw structs or compiler padding.
template<class IO,class T> void wildlife_totals_fields(IO& io,T& t) {
    io.field(t.births);io.field(t.deaths);io.field(t.hunted);io.field(t.meals);io.field(t.food);io.field(t.water);io.field(t.predation);io.field(t.spoiled);io.field(t.migrated);io.field(t.displaced_deaths);io.field(t.meat_created);
}
template<class IO,class S> void integrated_ecology_fields(IO& io,S& s) {
    for(auto& c:s.terrain.cells) {io.field(c.elevation);io.field(c.moisture);io.field(c.biome);}
    for(auto& d:s.minerals.deposits) {io.field(d.remaining);io.field(d.kind);io.field(d.discovered);}
    for(auto& c:s.drainage.cells) {io.field(c.downstream);io.field(c.area);io.field(c.runoff);}
    for(auto& t:s.wild.tiles) {for(auto& n:t.count) io.field(n);io.field(t.meat);}
    for(auto& n:s.wild.developed) io.field(n);
    for(auto& c:s.fish.cells) {io.field(c.habitat);io.field(c.fish);io.field(c.forage);}
    io.field(s.fish.initial);io.field(s.fish.births);io.field(s.fish.deaths);io.field(s.fish.caught);io.field(s.fish.forage_initial);io.field(s.fish.forage_grown);io.field(s.fish.forage_eaten);io.field(s.fish.last_pulse);io.field(s.fish.water_deaths);io.field(s.fish.food_deaths);io.field(s.fish.migrated);
    wildlife_totals_fields(io,s.ecology);
    io.field(s.initial_wild);
}
template<class IO,class S> void integrated_economy_fields(IO& io,S& s) {
    for(auto& n:s.knowledge.known) io.field(n);
    for(auto& r:s.knowledge.first) {io.field(r.tick);io.field(r.founder);}
    io.field(s.knowledge.job.owner);io.field(s.knowledge.job.tech);io.field(s.knowledge.job.work);
    flag(io,s.knowledge.job.active);
    io.field(s.knowledge.job.last_work_tick);
    io.field(s.knowledge.attempts);io.field(s.knowledge.successes);io.field(s.knowledge.failures);io.field(s.knowledge.abandoned);io.field(s.knowledge.last_opportunity);io.field(s.knowledge.last_teaching);
    for(auto& b:s.sites.sites) {io.field(b.stage);io.field(b.tile);io.field(b.owner);io.field(b.work);io.field(b.started);io.field(b.finished);io.field(b.last_work);}
    io.field(s.reserve.balance);io.field(s.reserve.deposited);io.field(s.reserve.withdrawn);io.field(s.reserve.spoiled);io.field(s.reserve.assisted);
    io.field(s.cooked.meals);io.field(s.cooked.wood);io.field(s.cooked.relief);
    io.field(s.sheltered.extra_recovery);
    io.field(s.home.steps);io.field(s.home.arrivals);
    for(auto& c:s.farms.crops) {
        io.field(c.stage);io.field(c.last_work);io.field(c.plants);io.field(c.tended);io.field(c.water);io.field(c.harvests);io.field(c.food);io.field(c.drought_losses);io.field(c.spoilage_losses);io.field(c.lost_water);io.field(c.dry_since);io.field(c.distributed);
        io.field(c.nutrients.fertility);io.field(c.nutrients.spent);io.field(c.nutrients.restored);io.field(c.nutrients.last_recovery);io.field(c.nutrients.amended);
    }
    io.field(s.water.last_transfer);io.field(s.water.transfers);io.field(s.water.water);
    for(auto& g:s.animals.groups) {io.field(g.count);io.field(g.captured);io.field(g.births);io.field(g.deaths);io.field(g.harvested);}
    wildlife_totals_fields(io,s.animals.resources);
    io.field(s.animals.meat_produced);io.field(s.animals.last_pulse);io.field(s.animals.supplied_food);io.field(s.animals.supplied_water);
    io.field(s.animals.fodder.balance);io.field(s.animals.fodder.deposited);io.field(s.animals.fodder.released);
    io.field(s.animals.organic.balance);
    for(auto& n:s.animals.organic.carried) io.field(n);
    io.field(s.animals.organic.feed);io.field(s.animals.organic.produced);io.field(s.animals.organic.applied);io.field(s.animals.organic.lost);
}
}
