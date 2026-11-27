#pragma once
#include "spatial_lookup.hpp"
#include "population.hpp"

namespace genesis::detail {
static_assert(live_capacity<65536);
// Packed key: tile in upper 16 bits, ascending live slot in lower 16.
// Only eligible nonbusy citizens enter the index; it is rebuilt per opportunity.
struct ParentTiles {
    std::array<std::uint32_t,live_capacity> keys{};
    std::size_t count{};
    // Consecutive parents on the same tile share the immutable neighborhood.
    mutable std::uint16_t cached_tile{0xffffu};
    mutable std::array<std::uint32_t,(live_capacity+31)/32> bits{};
    void add(const Citizen& c,std::size_t slot) noexcept {
        keys[count++]=(std::uint32_t(tile_key(c))<<16)|static_cast<std::uint32_t>(slot);
    }
    void sort() noexcept {std::sort(keys.begin(),keys.begin()+count);cached_tile=0xffffu;}
    template<class Visit>
    void later_neighbors(const Citizen& c,std::size_t slot,const Visit& visit) const noexcept {
        // Gather all nearby eligible slots once per consecutive tile, then
        // mask earlier slots and enumerate in the original ascending order.
        if(cached_tile!=tile_key(c)) {
            bits.fill(0);
            local_rows(c,2,[&](std::uint16_t first,std::uint16_t last) noexcept {
                const auto end=keys.begin()+count;
                auto it=std::lower_bound(keys.begin(),end,std::uint32_t(first)<<16);
                const auto upper=(std::uint32_t(last)<<16)|0xffffu;
                for(;it!=end&&*it<=upper;++it) {
                    const auto j=std::size_t(*it&0xffffu);
                    bits[j/32]|=std::uint32_t(1)<<(j%32);
                }
            });
            cached_tile=tile_key(c);
        }
        if(slot<2) {
            // Founders may only pair with one another.
            if(slot==0&&(bits[0]&2u)) visit(std::size_t{1});
            return;
        }
        for(std::size_t word=slot/32;word<bits.size();++word) {
            auto mask=bits[word];
            if(word==slot/32) {
                const auto self=std::uint32_t(1)<<(slot%32);
                mask&=~(self|(self-1u));
            }
            while(mask) {
                // GCC/Clang intrinsic: defined for nonzero input only.
                const auto bit=static_cast<unsigned>(__builtin_ctz(mask));
                visit(word*32+bit);
                mask&=mask-1;
            }
        }
    }
};
}
