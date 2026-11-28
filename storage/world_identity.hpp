#pragma once
#include "byte_codec.hpp"
#include <array>
#include <algorithm>
namespace genesis::storage::world_identity {
// Identity is independent of the simulation seed and RNG streams.
// A physical maintenance adapter supplies random UUID bytes at explicit Genesis.
using Uuid=std::array<std::uint8_t,16>;
struct Identity {
    Uuid uuid{};
    std::uint64_t created_unix_seconds{};
    bool official{};
};
inline bool valid(const Identity& id) noexcept {
    return (id.uuid[6]&0xf0u)==0x40u&&(id.uuid[8]&0xc0u)==0x80u&&
        (!id.official||id.created_unix_seconds!=0);
}
inline bool equal(const Identity& a,const Identity& b) noexcept {
    return a.uuid==b.uuid&&a.created_unix_seconds==b.created_unix_seconds&&a.official==b.official;
}
// Caller supplies independent entropy, never a seeded simulation stream.
inline Identity make(Uuid entropy,std::uint64_t created,bool official) {
    if(std::all_of(entropy.begin(),entropy.end(),[](auto n){return n==0;}))
        throw std::runtime_error("world identity entropy unavailable");
    entropy[6]=static_cast<std::uint8_t>((entropy[6]&0x0fu)|0x40u);
    entropy[8]=static_cast<std::uint8_t>((entropy[8]&0x3fu)|0x80u);
    Identity id{entropy,created,official};
    if(!valid(id)) throw std::runtime_error("invalid world creation metadata");
    return id;
}
}
