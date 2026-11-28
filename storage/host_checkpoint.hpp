#pragma once
#include "checkpoint.hpp"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace genesis::storage {
// Host POSIX adapter only. Caller serializes access in an isolated world directory.
struct FileRead {Bytes bytes;bool missing{};};
inline std::runtime_error io_error(const char* operation) {
    return std::runtime_error(std::string(operation)+": "+std::strerror(errno));
}
struct Fd {
    int value{-1};
    explicit Fd(int fd):value(fd) {}
    Fd(const Fd&)=delete;
    Fd& operator=(const Fd&)=delete;
    ~Fd(){if(value>=0) ::close(value);}
    void close_checked() {
        const int fd=value;value=-1;
        if(::close(fd)!=0) throw io_error("close");
    }
};
inline FileRead read_file(const std::string& path) {
    Fd fd(::open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK));
    if(fd.value<0) {
        if(errno==ENOENT) return {{},true};
        throw io_error("open checkpoint");
    }
    struct stat st{};
    if(::fstat(fd.value,&st)!=0) throw io_error("stat checkpoint");
    if(!S_ISREG(st.st_mode)) throw std::runtime_error("checkpoint is not a regular file");
    if(st.st_size<0||static_cast<std::uint64_t>(st.st_size)>checkpoint_limit) return {{},false};
    Bytes bytes(static_cast<std::size_t>(st.st_size));
    std::size_t offset=0;
    while(offset<bytes.size()) {
        const auto n=::read(fd.value,bytes.data()+offset,bytes.size()-offset);
        if(n<0&&errno==EINTR) continue;
        if(n<0) throw io_error("read checkpoint");
        if(!n) return {{},false}; // truncated concurrently, treat as invalid
        offset+=static_cast<std::size_t>(n);
    }
    std::uint8_t extra{};ssize_t n;
    do {n=::read(fd.value,&extra,1);} while(n<0&&errno==EINTR);
    if(n<0) throw io_error("check checkpoint length");
    if(n) return {{},false};
    return {std::move(bytes),false};
}
inline std::string slot_path(const std::string& directory,int slot) {
    return directory+(slot==0?"/world-a.pgcp":"/world-b.pgcp");
}
inline Recovery load_host(const std::string& directory) {
    try {
        const auto a=read_file(slot_path(directory,0)),b=read_file(slot_path(directory,1));
        return recover(a.bytes,b.bytes);
    } catch(const std::exception& e) {Recovery r{};r.state.error=e.what();return r;}
}
inline void publish_file(const std::string& directory,int slot,const Bytes& bytes) {
    Fd dir(::open(directory.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC));
    if(dir.value<0) throw io_error("open checkpoint directory");
    std::string temporary=directory+"/.checkpoint-XXXXXX";
    Fd fd(::mkstemp(temporary.data()));
    if(fd.value<0) throw io_error("create temporary checkpoint");
    try {
        std::size_t offset=0;
        while(offset<bytes.size()) {
            const auto n=::write(fd.value,bytes.data()+offset,bytes.size()-offset);
            if(n<0&&errno==EINTR) continue;
            if(n<=0) throw io_error("write checkpoint");
            offset+=static_cast<std::size_t>(n);
        }
        if(::fsync(fd.value)!=0) throw io_error("sync checkpoint");
        fd.close_checked();
        if(read_file(temporary).bytes!=bytes) throw std::runtime_error("checkpoint readback differs");
        if(::rename(temporary.c_str(),slot_path(directory,slot).c_str())!=0) throw io_error("publish checkpoint");
        if(::fsync(dir.value)!=0) throw io_error("sync checkpoint directory");
    } catch(...) {::unlink(temporary.c_str());throw;}
}
struct SaveResult {std::uint64_t generation{};std::string error;};
inline SaveResult save_host(const std::string& directory,const World& w,bool allow_create=false) {
    try {
        const auto a=read_file(slot_path(directory,0)),b=read_file(slot_path(directory,1));
        auto prior=recover(a.bytes,b.bytes);
        std::uint64_t generation=1;int slot=0;
        if(prior.state.world) {
            if(prior.state.world->seed!=w.seed||prior.state.world->tick>w.tick)
                throw std::runtime_error("world seed or time conflict");
            if(prior.state.world->tick==w.tick&&encode(*prior.state.world,1)!=encode(w,1))
                throw std::runtime_error("different state at the same tick");
            if(prior.state.generation==std::numeric_limits<std::uint64_t>::max())
                throw std::runtime_error("checkpoint generation exhausted");
            generation=prior.state.generation+1;slot=1-prior.slot;
        } else if(!allow_create||!a.missing||!b.missing) {
            throw std::runtime_error("refusing to overwrite missing, corrupt or conflicting checkpoints: "+prior.state.error);
        }
        publish_file(directory,slot,encode(w,generation));
        return {generation,{}};
    } catch(const std::exception& e) {return {0,e.what()};}
}
} // namespace genesis::storage
