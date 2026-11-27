#pragma once
#include "archive_replay.hpp"

namespace genesis::storage {
// Single writer, isolated existing directory, trusted paths. No legacy mixing.
inline ArchiveState load_archive_host(const std::string& dir) {
    const auto a=read_file(slot_path(dir,0)),b=read_file(slot_path(dir,1));
    ArchiveState x,y;
    if(intact_archive_bytes(a.bytes)) {x=decode_archive_checkpoint(dir,a.bytes);x.slot=0;}
    if(intact_archive_bytes(b.bytes)) {y=decode_archive_checkpoint(dir,b.bytes);y.slot=1;}
    if(!x.world&&!y.world) throw std::runtime_error("no verified archive checkpoint");
    if(x.world&&y.world) {
        if(x.generation<y.generation) {
            if(!valid_archive_pair(x,y)) throw std::runtime_error("archive generations conflict");
            return y;
        }
        if(!valid_archive_pair(y,x)) throw std::runtime_error("archive generations conflict");
        return x;
    }
    // A missing peer is expected only for the first generation. Any other
    // fallback is read-only; don't reuse an ordinal from an abandoned history.
    if(x.world) {x.degraded=!(x.generation==1&&b.missing);return x;}
    y.degraded=!(y.generation==1&&a.missing);return y;
}
inline ArchiveState create_archive_host(const std::string& dir,const World& original) {
    if(!valid_population(original)) throw std::runtime_error("legacy world invalid");
    if(!read_file(slot_path(dir,0)).missing||!read_file(slot_path(dir,1)).missing)
        throw std::runtime_error("archive destination already contains a checkpoint");
    auto index=DiskHistoryIndex::load(dir,original.seed,0,original.next_citizen_id);
    publish_file(dir,0,encode_archive_checkpoint(original,index,1));
    return load_archive_host(dir);
}
// Reject stale or changed caller state before publication.
inline ArchiveState require_current_archive(const std::string& dir,const ArchiveState& current) {
    if(!current.world||!current.history||current.degraded||
       current.generation==std::numeric_limits<std::uint64_t>::max())
        throw std::runtime_error("archive operation requires a verified state");
    auto disk=load_archive_host(dir);
    if(disk.degraded||disk.slot!=current.slot||disk.generation!=current.generation||
       disk.committed!=current.committed||
       encode_archive_checkpoint(*current.world,*current.history,current.generation)!=disk.committed)
        throw std::runtime_error("stale or changed archive checkpoint; reload required");
    return disk;
}
// Publish immutable segments first. Publish the new checkpoint second. Return
// the replacement state only after disk recovery verifies it. The caller's old
// state and previous slot are untouched even if a write throws.
inline ArchiveState retire_archive_host(const std::string& dir,const ArchiveState& current,std::size_t count) {
    auto disk=require_current_archive(dir,current);
    if(!count||count>disk.world->archive_count)
        throw std::runtime_error("invalid retirement count");
    const auto prefix=disk.history->count();
    for(std::size_t offset=0;offset<count;) {
        const auto n=std::min(segment_capacity,count-offset);
        HistorySegment batch{disk.world->seed,prefix+offset,
            {disk.world->archive.begin()+offset,disk.world->archive.begin()+offset+n}};
        publish_segment(dir,batch);
        offset+=n;
    }
    auto candidate=std::make_unique<World>(*disk.world);
    remove_resident_prefix(*candidate,count);
    auto index=DiskHistoryIndex::load(dir,candidate->seed,prefix+count,candidate->next_citizen_id);
    const auto bytes=encode_archive_checkpoint(*candidate,index,disk.generation+1);
    publish_file(dir,1-disk.slot,bytes);
    auto verified=load_archive_host(dir);
    if(verified.degraded||verified.committed!=bytes)
        throw std::runtime_error("archive publication uncertain; reload required");
    return verified;
}
// Advance only from exact disk state; no arbitrary candidate accepted.
inline ArchiveState advance_archive_host(const std::string& dir,const ArchiveState& current,std::uint64_t ticks) {
    if(!ticks||ticks>archive_replay_limit) throw std::runtime_error("invalid archive replay window");
    auto disk=require_current_archive(dir,current);
    if(!read_file(history_path(dir,disk.history->count())).missing)
        throw std::runtime_error("uncommitted history batch; complete retirement first");
    auto candidate=std::make_unique<World>(*disk.world);
    if(!replay_archive(*candidate,*disk.history,ticks)) throw std::runtime_error("archive replay rejected");
    const auto bytes=encode_archive_checkpoint(*candidate,*disk.history,disk.generation+1);
    publish_file(dir,1-disk.slot,bytes);
    auto verified=load_archive_host(dir);
    if(verified.degraded||verified.committed!=bytes)
        throw std::runtime_error("archive advancement uncertain; reload required");
    return verified;
}
} // namespace genesis::storage
