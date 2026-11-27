#pragma once
#include "host_history.hpp"
#include <sys/types.h>

namespace genesis::storage {
// Derive canonical row width from the existing codec, not native padding.
inline Bytes disk_row(const DeathRecord& d) {
    Writer out;citizen_fields(out,d.citizen);out.field(d.death_tick);out.field(d.cause);
    out.field(crc32(out.bytes,out.bytes.size()));return std::move(out.bytes);
}
inline std::size_t disk_row_size() {
    static const auto size=disk_row(DeathRecord{}).size();return size;
}
inline off_t disk_row_offset(std::uint64_t id) {
    const auto size=disk_row_size();
    if(id<3||(id-3)>static_cast<std::uint64_t>(std::numeric_limits<off_t>::max())/size-1)
        throw std::runtime_error("disk history row offset exhausted");
    return static_cast<off_t>((id-3)*size);
}
inline void write_disk_row(int fd,const DeathRecord& d) {
    const auto bytes=disk_row(d);
    const auto start=disk_row_offset(d.citizen.id);
    std::size_t done=0;
    while(done<bytes.size()) {
        const auto n=::pwrite(fd,bytes.data()+done,bytes.size()-done,start+static_cast<off_t>(done));
        if(n<0&&errno==EINTR) continue;
        if(n<=0) throw io_error("write history scratch row");
        done+=static_cast<std::size_t>(n);
    }
}
inline bool read_disk_row(int fd,std::uint64_t id,DeathRecord& result) {
    Bytes bytes(disk_row_size());
    const auto start=disk_row_offset(id);
    std::size_t done=0;
    while(done<bytes.size()) {
        const auto n=::pread(fd,bytes.data()+done,bytes.size()-done,start+static_cast<off_t>(done));
        if(n<0&&errno==EINTR) continue;
        if(n<0) throw io_error("read history scratch row");
        if(n==0) throw std::runtime_error("truncated history scratch index");
        done+=static_cast<std::size_t>(n);
    }
    // Sparse holes represent identities not in the external death prefix.
    if(std::all_of(bytes.begin(),bytes.end(),[](std::uint8_t b){return b==0;})) return false;
    const auto end=bytes.size()-4;
    Reader tail{bytes,end,bytes.size()};std::uint32_t crc{};tail.field(crc);
    if(crc!=crc32(bytes,end)) throw std::runtime_error("history scratch CRC mismatch");
    Reader in{bytes,0,end};DeathRecord d{};
    citizen_fields(in,d.citizen);in.field(d.death_tick);in.field(d.cause);
    if(in.pos!=end||d.citizen.id!=id) throw std::runtime_error("history scratch identity mismatch");
    result=d;return true;
}
} // namespace genesis::storage
