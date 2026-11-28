#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
namespace genesis::manure {
// Organic byproduct, not edible stock. Two consumed feed units produce
// one manure unit; the cumulative feed counter retains odd remainders.
inline constexpr unsigned capacity=64,carry_capacity=4,nutrient_value=4;
struct Store {
    std::uint16_t balance{};
    std::array<std::uint8_t,2> carried{};
    std::uint64_t feed{},produced{},applied{},lost{};
};
inline bool valid(const Store& s) noexcept {
    return s.balance<=capacity&&s.carried[0]<=carry_capacity&&
           s.carried[1]<=carry_capacity&&s.produced==s.feed/2&&
           s.produced==s.balance+s.carried[0]+s.carried[1]+s.applied+s.lost;
}
inline void record_feed(Store& s,unsigned fed) noexcept {
    s.feed+=fed;
    const auto made=s.feed/2-s.produced;
    s.produced+=made;
    const auto stored=std::min<std::uint64_t>(made,capacity-s.balance);
    s.balance+=stored;s.lost+=made-stored;
}
inline bool collect(Store& s,unsigned owner) noexcept {
    if(owner>=s.carried.size()||s.carried[owner]||!s.balance) return false;
    const auto n=std::min<unsigned>(carry_capacity,s.balance);
    s.balance-=n;s.carried[owner]=static_cast<std::uint8_t>(n);return true;
}
}
