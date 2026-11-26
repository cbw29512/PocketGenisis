#pragma once
#include <cstdint>

namespace genesis {
// SplitMix64: unsigned wraparound is defined, independent of host byte order.
// The entire stream state is one word; persist it for exact replay.
struct Rng {
    std::uint64_t state{};
    std::uint64_t next() noexcept {
        auto z = (state += UINT64_C(0x9e3779b97f4a7c15));
        z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
        z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
        return z ^ (z >> 31);
    }
    // Zero is not a valid range. Report it without consuming randomness.
    bool bounded(std::uint32_t limit, std::uint32_t& out) noexcept {
        if (!limit) return false;
        const auto n = std::uint64_t{limit};
        const auto threshold = (std::uint64_t{0} - n) % n;
        std::uint64_t value;
        do { value = next(); } while (value < threshold);
        out = static_cast<std::uint32_t>(value % n);
        return true;
    }
};
} // namespace genesis
