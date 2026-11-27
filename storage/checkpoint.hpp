#pragma once
#include "checkpoint_fields.hpp"
#include "population.hpp"
#include <memory>
#include <string>
#include <utility>

namespace genesis::storage {
inline constexpr std::uint32_t checkpoint_magic=0x50434750; // PGCP in little endian
inline constexpr std::uint16_t checkpoint_version=1, checkpoint_rules=14;
inline constexpr std::size_t checkpoint_limit=128*1024;
inline std::uint32_t crc32(const Bytes& bytes,std::size_t length) noexcept {
    std::uint32_t crc=0xffffffffu;
    for(std::size_t i=0;i<length;++i) {
        crc^=bytes[i];
        for(unsigned bit=0;bit<8;++bit) crc=(crc>>1)^((0u-(crc&1u))&0xedb88320u);
    }
    return ~crc;
}
inline Bytes encode(const World& w,std::uint64_t generation) {
    if(!generation||!valid_population(w)) throw std::runtime_error("invalid checkpoint state or generation");
    Writer out;out.bytes.reserve(checkpoint_limit);
    out.field(checkpoint_magic);out.field(checkpoint_version);out.field(checkpoint_rules);out.field(generation);
    world_fields(out,w);
    out.field(crc32(out.bytes,out.bytes.size()));
    if(out.bytes.size()>checkpoint_limit) throw std::runtime_error("checkpoint too large");
    return std::move(out.bytes);
}
struct Decoded {
    std::unique_ptr<World> world;
    std::uint64_t generation{};
    std::string error;
    bool unsupported{};
};
inline Decoded decode(const Bytes& bytes) {
    try {
        if(bytes.size()<20||bytes.size()>checkpoint_limit) throw std::runtime_error("checkpoint size invalid");
        const auto payload_end=bytes.size()-4;
        Reader tail{bytes,payload_end,bytes.size()};std::uint32_t expected{};tail.field(expected);
        if(expected!=crc32(bytes,payload_end)) throw std::runtime_error("checkpoint CRC mismatch");
        Reader in{bytes,0,payload_end};
        std::uint32_t magic{};std::uint16_t version{},rules{};std::uint64_t generation{};
        in.field(magic);in.field(version);in.field(rules);in.field(generation);
        if(magic!=checkpoint_magic) throw std::runtime_error("invalid checkpoint magic");
        if(version!=checkpoint_version||rules!=checkpoint_rules)
            return {nullptr,0,"unsupported checkpoint format or rules",true};
        if(!generation) throw std::runtime_error("invalid checkpoint generation");
        auto candidate=std::make_unique<World>();
        world_fields(in,*candidate);
        if(in.pos!=payload_end||!valid_population(*candidate)) throw std::runtime_error("checkpoint state invalid");
        return {std::move(candidate),generation,{}};
    } catch(const std::exception& e) {return {nullptr,0,e.what()};}
}
struct Recovery {
    Decoded state;
    int slot{-1};
    bool degraded{};
    std::string slot_errors[2];
};
inline Recovery recover(const Bytes& a,const Bytes& b) {
    auto x=decode(a),y=decode(b);
    Recovery result{};result.slot_errors[0]=x.error;result.slot_errors[1]=y.error;
    if(x.unsupported||y.unsupported) {result.state.error="unsupported checkpoint requires migration";result.state.unsupported=true;return result;}
    if(!x.world&&!y.world) {result.state.error="no valid checkpoint";return result;}
    if(x.world&&y.world) {
        if(x.world->seed!=y.world->seed||(x.generation==y.generation&&a!=b)||
           (x.generation>y.generation&&x.world->tick<y.world->tick)||
           (y.generation>x.generation&&y.world->tick<x.world->tick)) {
            result.state.error="conflicting checkpoint generations";return result;
        }
    } else result.degraded=true;
    result.slot=x.world&&(!y.world||x.generation>=y.generation)?0:1;
    result.state=result.slot==0?std::move(x):std::move(y);
    return result;
}
} // namespace genesis::storage
