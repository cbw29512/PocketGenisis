#pragma once
#include "checkpoint.hpp"

namespace genesis::storage {
// Immutable death-record batches. A segment is not a complete world checkpoint.
inline constexpr std::uint32_t history_magic=0x53484750; // PGHS
inline constexpr std::uint16_t history_version=1;
inline constexpr std::size_t segment_capacity=64;
struct HistorySegment {
    std::uint64_t seed{}, first{}; // Zero-based death-record ordinal.
    std::vector<DeathRecord> records;
};
inline bool valid_segment(const HistorySegment& s) noexcept {
    if(s.records.empty()||s.records.size()>segment_capacity||
       s.first>std::numeric_limits<std::uint64_t>::max()-s.records.size()) return false;
    for(std::size_t i=0;i<s.records.size();++i) {
        const auto& d=s.records[i];const auto& c=d.citizen;
        if(c.id<3||!valid_pair(c.parent1,c.parent2)||c.parent2>=c.id||
           !c.birth_tick||c.birth_tick>d.death_tick||c.lifespan_years<60||
           c.lifespan_years>80||!valid_agent(c)||
           static_cast<unsigned>(d.cause)<1||static_cast<unsigned>(d.cause)>5||
           (d.cause==DeathCause::old_age?c.health==0:c.health!=0)||
           (d.cause==DeathCause::disease&&!is_ill(c))||
           ((d.cause==DeathCause::childbirth)!=(c.birth_outcome==BirthOutcome::fatal))||
           (d.cause==DeathCause::childbirth&&d.death_tick!=c.birth_tick)) return false;
        if(i) {
            const auto& prev=s.records[i-1];
            if(d.death_tick<prev.death_tick||(d.death_tick==prev.death_tick&&c.id<=prev.citizen.id)) return false;
        }
        for(std::size_t j=0;j<i;++j) if(s.records[j].citizen.id==c.id) return false;
    }
    return true;
}
inline HistorySegment extract_segment(const World& w,std::size_t first,std::size_t count) {
    if(!valid_population(w)||first>w.archive_count||!count||count>segment_capacity||
       count>w.archive_count-first) throw std::runtime_error("invalid archive export range or world");
    return {w.seed,first,{w.archive.begin()+first,w.archive.begin()+first+count}};
}
inline Bytes encode_segment(const HistorySegment& s) {
    if(!valid_segment(s)) throw std::runtime_error("invalid death segment");
    Writer out;out.field(history_magic);out.field(history_version);out.field(checkpoint_rules);
    out.field(s.seed);out.field(s.first);out.count(s.records.size(),segment_capacity);
    for(const auto& d:s.records) {citizen_fields(out,d.citizen);out.field(d.death_tick);out.field(d.cause);}
    out.field(crc32(out.bytes,out.bytes.size()));
    return std::move(out.bytes);
}
inline HistorySegment decode_segment(const Bytes& bytes) {
    if(bytes.size()<30||bytes.size()>checkpoint_limit) throw std::runtime_error("invalid segment size");
    const auto end=bytes.size()-4;
    Reader tail{bytes,end,bytes.size()};std::uint32_t crc{};tail.field(crc);
    if(crc!=crc32(bytes,end)) throw std::runtime_error("history CRC mismatch");
    Reader in{bytes,0,end};std::uint32_t magic{};std::uint16_t version{},rules{};
    in.field(magic);in.field(version);in.field(rules);
    if(magic!=history_magic||version!=history_version||rules!=checkpoint_rules)
        throw std::runtime_error("unsupported history format or rules");
    HistorySegment s;std::size_t count{};in.field(s.seed);in.field(s.first);in.count(count,segment_capacity);
    s.records.resize(count);
    for(auto& d:s.records) {citizen_fields(in,d.citizen);in.field(d.death_tick);in.field(d.cause);}
    if(in.pos!=end||!valid_segment(s)) throw std::runtime_error("invalid death segment payload");
    return s;
}
} // namespace genesis::storage
