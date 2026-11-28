#pragma once
#include "integrated_recovery.hpp"
#include "world_identity.hpp"
namespace genesis::storage::identified_checkpoint {
// Versioned envelope preserves the existing PGCI codec and binds identity
// to the complete payload, including its generation, under an outer CRC.
inline constexpr std::uint32_t magic=0x44494750; // PGID
inline constexpr std::uint16_t version=1;
inline constexpr std::size_t overhead=39;
inline constexpr std::size_t limit=integrated_checkpoint::limit+overhead;
struct Decoded {
    world_identity::Identity identity{};
    Bytes payload{};
    std::string error;
    bool valid{},unsupported{};
};
inline Bytes encode(const world_identity::Identity& id,const integrated::Simulation& sim,
                    std::uint64_t generation) {
    if(!world_identity::valid(id)) throw std::runtime_error("invalid world identity");
    const auto payload=integrated_checkpoint::encode(sim,generation);
    Writer out;out.bytes.reserve(payload.size()+overhead);
    out.field(magic);out.field(version);
    for(auto b:id.uuid) out.field(b);
    out.field(id.created_unix_seconds);
    out.field(static_cast<std::uint8_t>(id.official));
    out.field(static_cast<std::uint32_t>(payload.size()));
    out.bytes.insert(out.bytes.end(),payload.begin(),payload.end());
    out.field(crc32(out.bytes,out.bytes.size()));
    return std::move(out.bytes);
}
inline Decoded decode(const Bytes& bytes) {
    Decoded result{};
    try {
        if(bytes.size()<overhead||bytes.size()>limit) throw std::runtime_error("invalid identity envelope size");
        const auto end=bytes.size()-4;
        Reader tail{bytes,end,bytes.size()};std::uint32_t checksum{};tail.field(checksum);
        if(checksum!=crc32(bytes,end)) throw std::runtime_error("identity envelope CRC mismatch");
        Reader in{bytes,0,end};std::uint32_t m{},n{};std::uint16_t v{};
        in.field(m);in.field(v);
        if(m!=magic) throw std::runtime_error("identity envelope magic mismatch");
        if(v!=version){result.unsupported=true;throw std::runtime_error("identity envelope migration required");}
        for(auto& b:result.identity.uuid) in.field(b);
        in.field(result.identity.created_unix_seconds);
        std::uint8_t official{};in.field(official);in.field(n);
        if(official>1||n!=end-in.pos||n>integrated_checkpoint::limit)
            throw std::runtime_error("invalid identity envelope fields");
        result.identity.official=official!=0;
        if(!world_identity::valid(result.identity)) throw std::runtime_error("invalid world identity");
        result.payload.assign(bytes.begin()+in.pos,bytes.begin()+end);
        if(integrated_checkpoint::unsupported(result.payload)){
            result.unsupported=true;throw std::runtime_error("integrated payload migration required");
        }
        result.valid=true;
    }catch(const std::exception& e){result.error=e.what();result.payload.clear();}
    return result;
}
struct Recovery {
    integrated_checkpoint::Recovery checkpoint{};
    world_identity::Identity identity{};
};
inline Recovery recover(const Bytes& a,const Bytes& b,const world_identity::Identity& expected) {
    Recovery out{};
    try {
        if(!world_identity::valid(expected)) throw std::runtime_error("expected world identity invalid");
        auto x=decode(a),y=decode(b);
        if(x.unsupported||y.unsupported){
            out.checkpoint.migration_required=true;throw std::runtime_error("checkpoint migration required");
        }
        // Never fall back to another world, even if its payload is otherwise valid.
        if((x.valid&&!world_identity::equal(x.identity,expected))||
           (y.valid&&!world_identity::equal(y.identity,expected)))
            throw std::runtime_error("checkpoint world identity conflict");
        out.checkpoint=integrated_checkpoint::recover(x.payload,y.payload);
        if(out.checkpoint.state.simulation) out.identity=expected;
    }catch(const std::exception& e){out.checkpoint.state.error=e.what();}
    return out;
}
}
