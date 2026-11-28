#pragma once
#include "../core/integrated_world.hpp"
#include "integrated_ecology_fields.hpp"
#include <memory>
#include <string>
#include <utility>
namespace genesis::storage::integrated_checkpoint {
inline constexpr std::uint32_t magic=0x49434750; // PGCI, distinct from legacy PGCP
inline constexpr std::uint16_t version=1,rules=1;
inline constexpr std::size_t limit=512*1024;
static_assert(sizeof(unsigned)==4,"checkpoint unsigned fields require 32-bit width");
template<class IO,class S,class O> void fields(IO& io,S& s,O& o) {
    world_fields<true>(io,s.world);
    integrated_ecology_fields(io,s);
    integrated_economy_fields(io,s);
    io.field(s.cadence);
    auto& f=s.founding;
    io.field(f.requested_seed);io.field(f.accepted_seed);io.field(f.attempts);
    flag(io,f.accepted);
    auto& r=f.report;
    for(auto& n:r.biomes) io.field(n);
    for(auto& n:r.ore) io.field(n);
    for(auto& n:r.groups) io.field(n);
    for(auto& n:r.water_regions) io.field(n);
    for(auto& n:r.fish_regions) io.field(n);
    io.field(r.fish_tiles);io.field(r.founder_pairs);io.field(r.wet_sinks);
    tile_index(io,r.spawn_a);tile_index(io,r.spawn_b);
    flag(io,r.valid_data);flag(io,r.accepted);
    io.field(o.production.food);io.field(o.production.water);io.field(o.production.wood);
    io.field(o.discovery_event.kind);io.field(o.discovery_event.tech);io.field(o.discovery_event.founder);
    io.field(o.construction_event.kind);io.field(o.construction_event.type);io.field(o.construction_event.tile);io.field(o.construction_event.builder);
}
inline bool valid_observation(const integrated::Observation& o) noexcept {
    return unsigned(o.discovery_event.kind)<=unsigned(discovery::EventKind::abandoned)&&
           o.discovery_event.tech<discovery::tech_count&&o.discovery_event.founder<=2&&
           unsigned(o.construction_event.kind)<=unsigned(construction::EventKind::resumed)&&
           o.construction_event.type<construction::kind_count&&
           o.construction_event.tile<std::size_t(width)*height&&o.construction_event.builder<=2;
}
inline Bytes encode(const integrated::Simulation& sim,std::uint64_t generation) {
    if(!generation||!sim.ready()||!integrated::valid(sim.state())||
       !valid_observation(sim.observation())) throw std::runtime_error("invalid integrated checkpoint");
    Writer out;out.bytes.reserve(limit);
    out.field(magic);out.field(version);out.field(rules);out.field(generation);
    fields(out,sim.state(),sim.observation());
    out.field(crc32(out.bytes,out.bytes.size()));
    if(out.bytes.size()>limit) throw std::runtime_error("integrated checkpoint too large");
    return std::move(out.bytes);
}
struct Decoded {
    std::unique_ptr<integrated::Simulation> simulation;
    std::uint64_t generation{};
    std::string error;
};
inline Decoded decode(const Bytes& bytes) {
    try {
        if(bytes.size()<20||bytes.size()>limit) throw std::runtime_error("invalid integrated checkpoint size");
        const auto end=bytes.size()-4;
        Reader tail{bytes,end,bytes.size()};std::uint32_t checksum{};tail.field(checksum);
        if(checksum!=crc32(bytes,end)) throw std::runtime_error("integrated checkpoint CRC mismatch");
        Reader in{bytes,0,end};
        std::uint32_t m{};std::uint16_t v{},r{};std::uint64_t generation{};
        in.field(m);in.field(v);in.field(r);in.field(generation);
        if(m!=magic||v!=version||r!=rules||!generation)
            throw std::runtime_error("unsupported integrated checkpoint format, rules or generation");
        auto state=std::make_unique<integrated::State>();
        integrated::Observation observation{};
        fields(in,*state,observation);
        if(in.pos!=end||!valid_observation(observation)) throw std::runtime_error("invalid integrated payload");
        auto sim=std::make_unique<integrated::Simulation>();
        if(!sim->restore(*state,observation)) throw std::runtime_error("invalid integrated world state");
        return {std::move(sim),generation,{}};
    }catch(const std::exception& e) {return {nullptr,0,e.what()};}
}
}
