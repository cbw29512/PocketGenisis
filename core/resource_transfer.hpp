#pragma once
#include <algorithm>
#include <cstdint>

namespace genesis::resource_transfer {
// Common stock handoff. Caller verifies locality, ownership and action.
// Preserve actual stock and both bounds; never count this as production.
inline unsigned move(std::uint16_t& source,std::uint16_t& dest,
                     unsigned reserve,unsigned target,unsigned batch) noexcept {
    if(&source==&dest||source<=reserve||dest>=target) return 0;
    const auto n=std::min({batch,unsigned(source-reserve),unsigned(target-dest)});
    source-=n;dest+=n;return n;
}
}
