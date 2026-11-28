#pragma once
#include "hydrology.hpp"
namespace genesis::walkability {
// Explicit terrain input: runoff and resource stock are NOT water depth.
enum class Surface : std::uint8_t { land, shallow, deep };
inline constexpr auto count=hydrology::tile_count;
inline constexpr auto blocked=hydrology::none;
using Map=std::array<Surface,count>;
struct Regions {
    std::array<std::uint16_t,count> label{};
    unsigned components{};
};
struct Scratch {std::array<std::uint16_t,count> queue{};};
inline bool walkable(Surface s) noexcept {
    return s==Surface::land||s==Surface::shallow;
}
// Caller owns scratch; bounded linear flood fill, no recursion or live RNG.
// Four-neighbor connectivity prevents diagonal jumps across water.
inline void classify(const Map& map,Regions& out,Scratch& scratch) noexcept {
    out.label.fill(blocked);out.components=0;
    for(std::size_t i=0;i<count;++i) {
        if(!walkable(map[i])||out.label[i]!=blocked) continue;
        const auto id=static_cast<std::uint16_t>(out.components++);
        std::size_t read=0,write=0;
        out.label[i]=id;scratch.queue[write++]=static_cast<std::uint16_t>(i);
        while(read<write) {
            const auto tile=scratch.queue[read++];
            hydrology::neighbors(tile,[&](std::size_t j) noexcept {
                if(!walkable(map[j])||out.label[j]!=blocked) return;
                out.label[j]=id;scratch.queue[write++]=static_cast<std::uint16_t>(j);
            });
        }
    }
}
inline bool connected(const Regions& r,std::size_t a,std::size_t b) noexcept {
    return a<count&&b<count&&r.label[a]!=blocked&&r.label[a]==r.label[b];
}
}
