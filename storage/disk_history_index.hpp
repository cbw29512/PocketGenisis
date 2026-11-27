#pragma once
#include "host_storage_budget.hpp"
#include "history_index.hpp"

namespace genesis::storage {
// Host prototype for an SD-backed index. The scratch file is unlinked on open;
// immutable PGHS segments remain the only historical authority. No RAM map.
class DiskHistoryIndex {
    std::unique_ptr<Fd> fd_;
    std::uint64_t seed_{},count_{},next_id_{};
    DeathRecord last_{};
    mutable DeathRecord lookup_{};
    DiskHistoryIndex()=default;
public:
    DiskHistoryIndex(const DiskHistoryIndex&)=delete;
    DiskHistoryIndex& operator=(const DiskHistoryIndex&)=delete;
    DiskHistoryIndex(DiskHistoryIndex&&)=default;
    static DiskHistoryIndex load(const std::string& dir,std::uint64_t seed,
                                 std::uint64_t count,std::uint64_t next_id) {
        if(next_id<3||count>next_id-3||next_id-3-count>history_capacity)
            throw std::runtime_error("invalid disk history identity bounds");
        require_scratch_storage(dir,next_id);
        DiskHistoryIndex index;index.seed_=seed;index.next_id_=next_id;
        std::string path=dir+"/.history-index-XXXXXX";
        // Allocate the owning wrapper before creating a file to avoid leaking fd.
        index.fd_=std::make_unique<Fd>(-1);
        index.fd_->value=::mkstemp(path.data());
        if(index.fd_->value<0) throw io_error("create history scratch index");
        if(::unlink(path.c_str())!=0) throw io_error("unlink history scratch index");
        const auto bytes=next_id==3?off_t{0}:disk_row_offset(next_id-1)+static_cast<off_t>(disk_row_size());
        if(::ftruncate(index.fd_->value,bytes)!=0) throw io_error("size history scratch index");
        // One segment and one lookup row are resident regardless of lifetime count.
        while(index.count_<count) {
            const auto segment=load_segment(dir,seed,index.count_);
            if(segment.records.size()>count-index.count_) throw std::runtime_error("history crosses committed boundary");
            for(const auto& d:segment.records) {
                if(d.citizen.id>=next_id) throw std::runtime_error("history identity outside assigned range");
                if(index.count_&&(index.last_.death_tick>d.death_tick||
                   (index.last_.death_tick==d.death_tick&&index.last_.citizen.id>=d.citizen.id)))
                    throw std::runtime_error("history chronology conflict");
                DeathRecord previous{};
                if(read_disk_row(index.fd_->value,d.citizen.id,previous))
                    throw std::runtime_error("duplicate historical identity");
                write_disk_row(index.fd_->value,d);
                index.last_=d;++index.count_;
            }
        }
        return index;
    }
    std::uint64_t seed() const noexcept {return seed_;}
    std::uint64_t count() const noexcept {return count_;}
    const DeathRecord* last() const noexcept {return count_?&last_:nullptr;}
    // Borrowed pointer valid until next find; shared genealogy consumes it
    // before the next lookup. Visit uses a different local record.
    const DeathRecord* find(std::uint64_t id) const {
        if(id<3||id>=next_id_) return nullptr;
        return read_disk_row(fd_->value,id,lookup_)?&lookup_:nullptr;
    }
    template<class Visit> bool visit(const Visit& visitor) const {
        std::uint64_t visited=0;
        for(std::uint64_t id=3;id<next_id_;++id) {
            DeathRecord row{};
            if(read_disk_row(fd_->value,id,row)) {
                ++visited;
                if(!visitor(row)) return false;
            }
        }
        if(visited!=count_) throw std::runtime_error("history scratch record count mismatch");
        return true;
    }
};
} // namespace genesis::storage
