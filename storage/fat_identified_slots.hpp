#pragma once
#include "identified_slots.hpp"
#include "identity_manifest.hpp"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
namespace genesis::storage::identified_checkpoint {
// ESP32 FAT VFS adapter. Caller must keep the verified card mounted and
// serialize access. Only the inactive checkpoint may be truncated.
// FAT directory updates are not assumed atomic; qualification must include
// physical interruptions. The other slot is never modified by publish().
class FatSlots {
    std::string dir_;
    static std::runtime_error error(const char* action){
        return std::runtime_error(std::string(action)+": "+std::strerror(errno));
    }
    struct File {
        int fd;
        explicit File(int n):fd(n){}
        File(const File&)=delete;
        File& operator=(const File&)=delete;
        ~File(){if(fd>=0)::close(fd);}
        void finish(){const int n=fd;fd=-1;if(::close(n)!=0)throw error("close");}
    };
    void require_directory() const {
        struct stat st{};
        if(::stat(dir_.c_str(),&st)!=0)throw error("stat world directory");
        if(!S_ISDIR(st.st_mode))throw std::runtime_error("world directory unavailable");
    }
    std::string path(int slot) const {
        if(slot!=0&&slot!=1)throw std::runtime_error("invalid slot");
        return dir_+(slot==0?"/world-a.pgid":"/world-b.pgid");
    }
    SlotRead read_path(const std::string& name,std::size_t maximum) const {
        require_directory();
        File f(::open(name.c_str(),O_RDONLY));
        if(f.fd<0){
            if(errno==ENOENT){require_directory();return {{},true};}
            throw error("open read");
        }
        struct stat st{};
        if(::fstat(f.fd,&st)!=0)throw error("stat file");
        if(!S_ISREG(st.st_mode))throw std::runtime_error("not regular file");
        if(st.st_size<0||static_cast<std::uint64_t>(st.st_size)>maximum)return {{},false};
        Bytes bytes(static_cast<std::size_t>(st.st_size));
        std::size_t pos=0;
        while(pos<bytes.size()){
            const auto n=::read(f.fd,bytes.data()+pos,std::min<std::size_t>(4096,bytes.size()-pos));
            if(n<0&&errno==EINTR)continue;
            if(n<0)throw error("read");
            if(!n)return {{},false};
            pos+=static_cast<std::size_t>(n);
        }
        std::uint8_t extra{};ssize_t n;
        do{n=::read(f.fd,&extra,1);}while(n<0&&errno==EINTR);
        if(n<0)throw error("verify length");
        if(n)return {{},false};
        f.finish();return {std::move(bytes),false};
    }
    void write_path(const std::string& name,const Bytes& bytes,int mode){
        require_directory();
        File f(::open(name.c_str(),O_WRONLY|O_CREAT|mode,0600));
        if(f.fd<0)throw error("open write");
        std::size_t pos=0;
        while(pos<bytes.size()){
            const auto n=::write(f.fd,bytes.data()+pos,std::min<std::size_t>(4096,bytes.size()-pos));
            if(n<0&&errno==EINTR)continue;
            if(n<=0)throw error("write");
            pos+=static_cast<std::size_t>(n);
        }
        if(::fsync(f.fd)!=0)throw error("sync");
        f.finish();
    }
public:
    explicit FatSlots(std::string directory):dir_(std::move(directory)){}
    SlotRead read(int slot) const {return read_path(path(slot),limit);}
    void publish(int slot,const Bytes& bytes){
        if(bytes.empty()||bytes.size()>limit)throw std::runtime_error("invalid checkpoint size");
        write_path(path(slot),bytes,O_TRUNC);
    }
    SlotRead manifest() const {return read_path(dir_+"/identity.pgim",identity_manifest::size);}
    void create_manifest(const world_identity::Identity& id){
        // Refuse to establish a new identity over any existing world.
        if(!manifest().missing||!read(0).missing||!read(1).missing)
            throw std::runtime_error("world directory already contains identity or checkpoints");
        const auto bytes=identity_manifest::encode(id);
        write_path(dir_+"/identity.pgim",bytes,O_EXCL);
        const auto checked=manifest();
        if(checked.missing||checked.bytes!=bytes)throw std::runtime_error("identity readback mismatch");
    }
};
}
