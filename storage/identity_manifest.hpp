#pragma once
#include "world_identity.hpp"
#include <string>
namespace genesis::storage::identity_manifest {
// Independent identity record, written once during explicit Genesis.
// Never infer an expected UUID from whichever checkpoint happens to load.
inline constexpr std::uint32_t magic=0x4d494750; // PGIM
inline constexpr std::uint16_t version=1;
inline constexpr std::size_t size=35;
struct Decoded {world_identity::Identity identity{};bool valid{};std::string error;};
inline Bytes encode(const world_identity::Identity& id){
    if(!world_identity::valid(id)) throw std::runtime_error("invalid identity manifest");
    Writer out;out.bytes.reserve(size);
    out.field(magic);out.field(version);
    for(auto b:id.uuid)out.field(b);
    out.field(id.created_unix_seconds);out.field(static_cast<std::uint8_t>(id.official));
    out.field(crc32(out.bytes,out.bytes.size()));return out.bytes;
}
inline Decoded decode(const Bytes& bytes){
    Decoded result{};
    try{
        if(bytes.size()!=size)throw std::runtime_error("identity manifest size mismatch");
        Reader tail{bytes,size-4,size};std::uint32_t checksum{};tail.field(checksum);
        if(checksum!=crc32(bytes,size-4))throw std::runtime_error("identity manifest CRC mismatch");
        Reader in{bytes,0,size-4};std::uint32_t m{};std::uint16_t v{};in.field(m);in.field(v);
        if(m!=magic||v!=version)throw std::runtime_error("unsupported identity manifest");
        world_identity::Identity id{};
        for(auto& b:id.uuid)in.field(b);
        std::uint8_t official{};in.field(id.created_unix_seconds);in.field(official);
        id.official=official==1;
        if(official>1||!world_identity::valid(id)||in.pos!=size-4)
            throw std::runtime_error("invalid identity manifest fields");
        result.identity=id;result.valid=true;
    }catch(const std::exception& e){result.error=e.what();}
    return result;
}
}
