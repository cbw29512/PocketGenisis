#pragma once
#include "disk_history_index.hpp"

namespace genesis::storage {
// Distinct host prototype format, never silently opened by the legacy loader.
inline constexpr std::uint32_t archive_checkpoint_magic=0x57414750; // PGAW
inline constexpr std::uint16_t archive_checkpoint_version=1;
struct ArchiveState {
    std::unique_ptr<World> world;
    std::unique_ptr<DiskHistoryIndex> history;
    std::uint64_t generation{};
    int slot{-1};
    bool degraded{};
    Bytes committed;
};
inline Bytes world_payload(const World& w) {
    Writer out;world_fields(out,w);return std::move(out.bytes);
}
inline Bytes death_payload(const DeathRecord& d) {
    Writer out;citizen_fields(out,d.citizen);out.field(d.death_tick);out.field(d.cause);
    return std::move(out.bytes);
}
template<class Index> inline Bytes encode_archive_checkpoint(const World& w,const Index& history,std::uint64_t generation) {
    if(!generation||!valid_history_view(w,history)) throw std::runtime_error("invalid archive checkpoint state");
    Writer out;out.field(archive_checkpoint_magic);out.field(archive_checkpoint_version);
    out.field(checkpoint_rules);out.field(generation);out.field(history.count());
    world_fields(out,w);out.field(crc32(out.bytes,out.bytes.size()));
    if(out.bytes.size()>checkpoint_limit) throw std::runtime_error("archive checkpoint too large");
    return std::move(out.bytes);
}
// CRC-invalid data is eligible for read-only fallback. A supported, checksummed
// checkpoint with missing history or semantic errors must instead fail closed.
inline bool intact_archive_bytes(const Bytes& bytes) {
    if(bytes.size()<28||bytes.size()>checkpoint_limit) return false;
    const auto end=bytes.size()-4;
    Reader tail{bytes,end,bytes.size()};std::uint32_t expected{};tail.field(expected);
    return expected==crc32(bytes,end);
}
inline ArchiveState decode_archive_checkpoint(const std::string& dir,const Bytes& bytes) {
    if(!intact_archive_bytes(bytes)) throw std::runtime_error("archive checkpoint CRC or size invalid");
    const auto end=bytes.size()-4;Reader in{bytes,0,end};
    std::uint32_t magic{};std::uint16_t version{},rules{};
    std::uint64_t generation{},prefix{};
    in.field(magic);in.field(version);in.field(rules);in.field(generation);in.field(prefix);
    if(magic!=archive_checkpoint_magic||version!=archive_checkpoint_version||rules!=checkpoint_rules||!generation)
        throw std::runtime_error("unsupported archive checkpoint or generation");
    auto w=std::make_unique<World>();world_fields(in,*w);
    if(in.pos!=end||w->next_citizen_id<3||prefix>w->next_citizen_id-3||
       w->next_citizen_id-3-prefix!=w->descendant_count+w->archive_count)
        throw std::runtime_error("archive checkpoint identity count invalid");
    auto history=std::make_unique<DiskHistoryIndex>(DiskHistoryIndex::load(dir,w->seed,prefix,w->next_citizen_id));
    if(!valid_history_view(*w,*history)) throw std::runtime_error("archive checkpoint semantics invalid");
    ArchiveState result;result.world=std::move(w);result.history=std::move(history);
    result.generation=generation;result.committed=bytes;return result;
}
inline void remove_resident_prefix(World& w,std::size_t count) {
    if(count>w.archive_count) throw std::runtime_error("invalid retirement count");
    for(std::size_t i=count;i<w.archive_count;++i) w.archive[i-count]=w.archive[i];
    for(std::size_t i=w.archive_count-count;i<w.archive_count;++i) w.archive[i]=DeathRecord{};
    w.archive_count-=count;
}
// Consecutive checkpoints in this prototype differ only by history retirement.
// Reconstruct the exact resident suffix and compare every transferred record.
inline bool valid_retirement_pair(const ArchiveState& older,const ArchiveState& newer) {
    if(older.generation==std::numeric_limits<std::uint64_t>::max()||
       newer.generation!=older.generation+1||newer.history->count()<=older.history->count()) return false;
    const auto count=newer.history->count()-older.history->count();
    if(count>older.world->archive_count) return false;
    for(std::size_t i=0;i<count;++i) {
        const auto& expected=older.world->archive[i];
        const auto* actual=newer.history->find(expected.citizen.id);
        if(!actual||death_payload(expected)!=death_payload(*actual)) return false;
    }
    auto remainder=std::make_unique<World>(*older.world);
    remove_resident_prefix(*remainder,static_cast<std::size_t>(count));
    return world_payload(*remainder)==world_payload(*newer.world);
}
} // namespace genesis::storage
