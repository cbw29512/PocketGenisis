#pragma once
#include <cstdint>
#include <cstddef>
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
inline std::uint32_t crc32(const Bytes& bytes,std::size_t length) noexcept {
    std::uint32_t crc=0xffffffffu;
    for(std::size_t i=0;i<length;++i) {
        crc^=bytes[i];
        for(unsigned bit=0;bit<8;++bit) crc=(crc>>1)^((0u-(crc&1u))&0xedb88320u);
    }
    return ~crc;
}
} // namespace genesis::storage
