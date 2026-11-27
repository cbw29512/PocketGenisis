#pragma once
#include "world.hpp"
#include <type_traits>
#include <vector>
#include <stdexcept>

namespace genesis::storage {
using Bytes=std::vector<std::uint8_t>;
// All scalar types have a specified width, independent of host padding/endian.
template<class T,bool IsEnum=std::is_enum_v<T>> struct Scalar {using type=T;};
template<class T> struct Scalar<T,true> {using type=std::underlying_type_t<T>;};
struct Writer {
    Bytes bytes;
    template<class T> void field(T value) {
        using U=typename Scalar<T>::type;
        static_assert(std::is_unsigned_v<U>);
        const auto v=static_cast<std::uint64_t>(value);
        for(std::size_t i=0;i<sizeof(U);++i) bytes.push_back(static_cast<std::uint8_t>(v>>(8*i)));
    }
    void count(std::size_t n,std::size_t cap) {
        if(n>cap) throw std::runtime_error("checkpoint count exceeds capacity");
        field(static_cast<std::uint16_t>(n));
    }
};
struct Reader {
    const Bytes& bytes;
    std::size_t pos{}, limit{};
    template<class T> void field(T& value) {
        using U=typename Scalar<T>::type;
        static_assert(std::is_unsigned_v<U>);
        if(pos>limit||sizeof(U)>limit-pos) throw std::runtime_error("truncated checkpoint");
        std::uint64_t v=0;
        for(std::size_t i=0;i<sizeof(U);++i) v|=std::uint64_t(bytes[pos++])<<(8*i);
        value=static_cast<T>(v);
    }
    void count(std::size_t& n,std::size_t cap) {
        std::uint16_t encoded{};field(encoded);
        if(encoded>cap) throw std::runtime_error("checkpoint count exceeds capacity");
        n=encoded;
    }
};
// One field order for writing and reading; changing it requires a version bump.
template<class IO,class Person> void citizen_fields(IO& io,Person& c) {
    io.field(c.id);io.field(c.x);io.field(c.y);
    for(auto& trait:c.personality) io.field(trait);
    io.field(c.hunger);io.field(c.thirst);io.field(c.action);
    io.field(c.parent1);io.field(c.parent2);io.field(c.birth_tick);
    io.field(c.guardian_id);io.field(c.guardian_since_tick);
    io.field(c.purpose);io.field(c.health);io.field(c.lifespan_years);io.field(c.birth_outcome);
    for(auto& d:c.diseases) {io.field(d.illness);io.field(d.immunity);io.field(d.resistance);}
}
template<class IO,class W> void world_fields(IO& io,W& w) {
    io.field(w.seed);io.field(w.tick);io.field(w.next_citizen_id);io.field(w.lost_gestations);
    for(auto& r:w.streams) io.field(r.state);
    for(auto& t:w.tiles) {io.field(t.food);io.field(t.wood);io.field(t.water);io.field(t.stone);}
    for(auto& f:w.founders) citizen_fields(io,f);
    io.count(w.descendant_count,descendant_capacity);
    for(std::size_t i=0;i<w.descendant_count;++i) citizen_fields(io,w.descendants[i]);
    io.count(w.archive_count,history_capacity);
    for(std::size_t i=0;i<w.archive_count;++i) {
        auto& d=w.archive[i];citizen_fields(io,d.citizen);io.field(d.death_tick);io.field(d.cause);
    }
    io.count(w.pregnancy_count,pregnancy_capacity);
    for(std::size_t i=0;i<w.pregnancy_count;++i) {
        auto& p=w.pregnancies[i];io.field(p.parent1);io.field(p.parent2);io.field(p.due_tick);
    }
}
} // namespace genesis::storage
