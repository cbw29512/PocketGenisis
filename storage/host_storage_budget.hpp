#pragma once
#include "disk_history_rows.hpp"
#include <sys/statvfs.h>

namespace genesis::storage {
// Advisory conservative preflight, not an exclusive filesystem reservation.
// The real filesystem reading is never replaced by the optional lower ceiling.
struct StorageBudget {
    std::uint64_t ceiling{std::numeric_limits<std::uint64_t>::max()};
};
struct StorageSpace {
    std::uint64_t available{},block{};
};
inline std::uint64_t budget_add(std::uint64_t a,std::uint64_t b) {
    if(b>std::numeric_limits<std::uint64_t>::max()-a)
        throw std::runtime_error("storage budget addition overflow");
    return a+b;
}
inline std::uint64_t budget_mul(std::uint64_t a,std::uint64_t b) {
    if(a&&b>std::numeric_limits<std::uint64_t>::max()/a)
        throw std::runtime_error("storage budget multiplication overflow");
    return a*b;
}
inline StorageSpace storage_space(const std::string& dir) {
    struct statvfs fs{};
    if(::statvfs(dir.c_str(),&fs)!=0) throw io_error("query storage capacity");
    if(!fs.f_frsize) throw std::runtime_error("unknown filesystem allocation unit");
    const auto block=static_cast<std::uint64_t>(fs.f_frsize);
    return {budget_mul(static_cast<std::uint64_t>(fs.f_bavail),block),block};
}
inline std::uint64_t scratch_bytes(std::uint64_t next_id) {
    if(next_id<3) throw std::runtime_error("invalid history identity bounds");
    if(next_id==3) return 0;
    // Also enforce the existing platform file-offset limit.
    return budget_add(static_cast<std::uint64_t>(disk_row_offset(next_id-1)),disk_row_size());
}
inline std::uint64_t round_storage(std::uint64_t bytes,std::uint64_t block) {
    if(!block) throw std::runtime_error("invalid storage allocation unit");
    return budget_mul(bytes/block+static_cast<std::uint64_t>(bytes%block!=0),block);
}
inline constexpr std::uint64_t emergency_storage_reserve=1024*1024;
inline std::uint64_t transaction_storage_bytes(std::uint64_t next_id,std::size_t retire_count,
                                                std::uint64_t block) {
    if(retire_count>history_capacity) throw std::runtime_error("invalid retirement budget");
    // Allow for scratch copies used by candidate generation and both recovery slots,
    // a temporary checkpoint, each immutable segment, and filesystem overhead.
    const auto future_id=budget_add(next_id,history_capacity);
    const auto scratch=budget_mul(4,round_storage(scratch_bytes(future_id),block));
    const auto checkpoints=budget_mul(2,round_storage(checkpoint_limit,block));
    const auto batches=retire_count/segment_capacity+(retire_count%segment_capacity!=0);
    const auto segments=budget_mul(batches,round_storage(checkpoint_limit,block));
    return budget_add(emergency_storage_reserve,
        budget_add(scratch,budget_add(checkpoints,budget_add(segments,budget_mul(16,block)))));
}
inline void require_storage(const std::string& dir,std::uint64_t next_id,std::size_t retire_count,
                            StorageBudget policy={}) {
    const auto space=storage_space(dir);
    const auto required=transaction_storage_bytes(next_id,retire_count,space.block);
    if(std::min(space.available,policy.ceiling)<required)
        throw std::runtime_error("insufficient storage: world paused before publication");
}
inline void require_scratch_storage(const std::string& dir,std::uint64_t next_id) {
    const auto space=storage_space(dir);
    if(space.available<budget_add(round_storage(scratch_bytes(next_id),space.block),space.block))
        throw std::runtime_error("insufficient storage for history validation");
}
} // namespace genesis::storage
