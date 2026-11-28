#pragma once
#include "identified_slots.hpp"
#include "host_checkpoint.hpp"
namespace genesis::storage::identified_checkpoint {
// POSIX reference adapter. Keep separate from the Arduino/FAT32 driver.
// Reuse the established checked file descriptor and error helpers.
class HostSlots {
    std::string directory_;
    std::string path(int slot) const {
        if(slot!=0&&slot!=1) throw std::runtime_error("invalid checkpoint slot");
        return directory_+(slot==0?"/world-a.pgid":"/world-b.pgid");
    }
public:
    explicit HostSlots(std::string directory):directory_(std::move(directory)) {}
    SlotRead read(int slot) const {
        const auto name=path(slot);
        Fd dir(::open(directory_.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC));
        if(dir.value<0) throw io_error("open world directory");
        Fd fd(::open(name.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK));
        if(fd.value<0){
            if(errno==ENOENT) return {{},true};
            throw io_error("open identified checkpoint");
        }
        struct stat st{};
        if(::fstat(fd.value,&st)!=0) throw io_error("stat identified checkpoint");
        if(!S_ISREG(st.st_mode)) throw std::runtime_error("checkpoint not regular file");
        if(st.st_size<0||static_cast<std::uint64_t>(st.st_size)>limit) return {{},false};
        Bytes bytes(static_cast<std::size_t>(st.st_size));
        std::size_t offset=0;
        while(offset<bytes.size()){
            const auto n=::read(fd.value,bytes.data()+offset,bytes.size()-offset);
            if(n<0&&errno==EINTR) continue;
            if(n<0) throw io_error("read identified checkpoint");
            if(!n) return {{},false};
            offset+=static_cast<std::size_t>(n);
        }
        std::uint8_t extra{};ssize_t n;
        do{n=::read(fd.value,&extra,1);}while(n<0&&errno==EINTR);
        if(n<0) throw io_error("check identified checkpoint length");
        if(n) return {{},false};
        return {std::move(bytes),false};
    }
    void publish(int slot,const Bytes& bytes) {
        const auto target=path(slot);
        if(bytes.empty()||bytes.size()>limit) throw std::runtime_error("invalid publication size");
        Fd dir(::open(directory_.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC));
        if(dir.value<0) throw io_error("open world directory");
        std::string temp=directory_+"/.identified-XXXXXX";
        Fd fd(::mkstemp(temp.data()));
        if(fd.value<0) throw io_error("create temporary identified checkpoint");
        try {
            std::size_t offset=0;
            while(offset<bytes.size()){
                const auto n=::write(fd.value,bytes.data()+offset,bytes.size()-offset);
                if(n<0&&errno==EINTR) continue;
                if(n<=0) throw io_error("write identified checkpoint");
                offset+=static_cast<std::size_t>(n);
            }
            if(::fsync(fd.value)!=0) throw io_error("sync identified checkpoint");
            fd.close_checked();
            if(::rename(temp.c_str(),target.c_str())!=0) throw io_error("publish identified checkpoint");
            if(::fsync(dir.value)!=0) throw io_error("sync world directory");
            // Generic transaction performs final exact readback.
        }catch(...){::unlink(temp.c_str());throw;}
    }
};
}
