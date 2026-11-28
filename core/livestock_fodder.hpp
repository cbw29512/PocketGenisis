#pragma once
#include "resource_transfer.hpp"
#include "wildlife.hpp"

namespace genesis::livestock {
// Separate physically gathered fodder inventory. Unlike edible tile stock,
// this is committed animal feed. Nothing is granted on entering winter.
inline constexpr unsigned fodder_capacity=192, winter_feed_pulses=18;
struct Fodder {
    std::uint16_t balance{};
    std::uint64_t deposited{},released{};
};
inline bool valid(const Fodder& f) noexcept {
    return f.balance<=fodder_capacity&&f.deposited==f.released+f.balance;
}
inline unsigned fodder_target(unsigned animals) noexcept {
    return std::min(fodder_capacity,winter_feed_pulses*std::max(2u,animals));
}
inline unsigned deposit_fodder(Fodder& f,Tile& source,unsigned animals,
                               unsigned reserve,unsigned batch) noexcept {
    const auto moved=resource_transfer::move(source.food,f.balance,reserve,
                                              fodder_target(animals),batch);
    f.deposited+=moved;return moved;
}
// Release only the shortfall for animals that can actually be watered.
// Every released unit is consumed by the existing grazing transaction.
inline unsigned feed_with_fodder(Tile& stock,unsigned requested,Fodder& f,
                                 wildlife::Totals& totals) noexcept {
    const auto support=std::min(requested,unsigned(stock.water));
    const auto moved=resource_transfer::move(f.balance,stock.food,0,support,fodder_capacity);
    f.released+=moved;
    return wildlife::graze(stock,requested,totals);
}
}
