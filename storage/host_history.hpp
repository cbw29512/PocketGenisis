#pragma once
#include "history_segment.hpp"
#include "host_checkpoint.hpp"

namespace genesis::storage {
// Caller owns an isolated, single-writer disposable world directory.
inline std::string history_path(const std::string& dir,std::uint64_t first) {
    return dir+"/history-"+std::to_string(first)+".pghs";
}
inline void publish_segment(const std::string& dir,const HistorySegment& segment) {
    const auto bytes=encode_segment(segment);
    const auto path=history_path(dir,segment.first);
    Fd directory(::open(dir.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC));
    if(directory.value<0) throw io_error("open history directory");
    const auto old=read_file(path);
    if(!old.missing) {
        if(old.bytes!=bytes) throw std::runtime_error("immutable history conflict");
        if(::fsync(directory.value)!=0) throw io_error("sync history directory");
        return;
    }
    std::string temp=dir+"/.history-XXXXXX";
    Fd fd(::mkstemp(temp.data()));
    if(fd.value<0) throw io_error("create history temporary file");
    try {
        std::size_t offset=0;
        while(offset<bytes.size()) {
            const auto n=::write(fd.value,bytes.data()+offset,bytes.size()-offset);
            if(n<0&&errno==EINTR) continue;
            if(n<=0) throw io_error("write history segment");
            offset+=static_cast<std::size_t>(n);
        }
        if(::fsync(fd.value)!=0) throw io_error("sync history segment");
        fd.close_checked();
        if(read_file(temp).bytes!=bytes) throw std::runtime_error("history readback differs");
        // Hard-link publication is create-only; unlike rename it cannot overwrite history.
        if(::link(temp.c_str(),path.c_str())!=0) throw io_error("publish immutable history");
        if(::unlink(temp.c_str())!=0) throw io_error("remove history temporary file");
        if(::fsync(directory.value)!=0) throw io_error("sync history directory");
    } catch(...) {::unlink(temp.c_str());throw;}
}
inline HistorySegment load_segment(const std::string& dir,std::uint64_t seed,std::uint64_t first) {
    const auto data=read_file(history_path(dir,first));
    if(data.missing) throw std::runtime_error("missing history segment");
    auto s=decode_segment(data.bytes);
    if(s.seed!=seed||s.first!=first) throw std::runtime_error("history identity or ordinal conflict");
    return s;
}
} // namespace genesis::storage
