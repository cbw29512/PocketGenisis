#pragma once
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <stdexcept>
namespace genesis::maintenance {
// Fixed allowlist for the disposable checkpoint layout. Never traverse or
// recursively delete the SD root. Unknown files cause refusal, not removal.
template<class IdentityReader>
class ResetFiles {
    std::string dir_,marker_;
    IdentityReader read_identity_;
    static void fail(const char* message){throw std::runtime_error(message);}
    static bool exists(const std::string& path){
        struct stat st{};
        if(::stat(path.c_str(),&st)==0)return true;
        if(errno==ENOENT)return false;
        fail("reset path stat failed");return false;
    }
    static bool valid_id(const std::string& id){
        if(id.size()!=32)return false;
        for(char c:id)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;
        return true;
    }
    static void remove_file(const std::string& path){
        if(::unlink(path.c_str())!=0&&errno!=ENOENT)fail("reset unlink failed");
        if(exists(path))fail("reset file still present");
    }
    bool scan(bool empty_only)const{
        DIR* d=::opendir(dir_.c_str());
        if(!d)fail("reset directory open failed");
        bool empty=true,allowed=true;
        errno=0;
        while(auto* entry=::readdir(d)){
            const std::string n=entry->d_name;
            if(n=="."||n=="..")continue;
            empty=false;
            if(n!="identity.pgim"&&n!="world-a.pgid"&&n!="world-b.pgid")allowed=false;
        }
        const int e=errno;::closedir(d);
        if(e||!allowed)fail("reset directory contains unknown files or could not be read");
        return !empty_only||empty;
    }
public:
    ResetFiles(std::string dir,IdentityReader reader):dir_(std::move(dir)),
        marker_(dir_+".reset"),read_identity_(std::move(reader)){}
    bool pending()const{return exists(marker_);}
    bool world_absent()const{return !exists(dir_);}
    std::string identity()const{return read_identity_();}
    void preflight()const{
        if(world_absent()||!valid_id(identity()))fail("reset identity unavailable");
        scan(false);
    }
    std::string reset_identity()const{
        const int fd=::open(marker_.c_str(),O_RDONLY);
        if(fd<0)fail("reset marker unavailable");
        char bytes[33]{};
        const auto n=::read(fd,bytes,sizeof(bytes));
        const int closed=::close(fd);
        if(n!=32||closed!=0)fail("reset marker malformed");
        const std::string id(bytes,32);
        if(!valid_id(id))fail("reset marker identity invalid");
        return id;
    }
    void begin_reset(const std::string& id){
        if(!valid_id(id)||identity()!=id)fail("reset identity changed");
        preflight();
        const int fd=::open(marker_.c_str(),O_WRONLY|O_CREAT|O_EXCL,0600);
        if(fd<0)fail("reset marker creation failed");
        const auto n=::write(fd,id.data(),id.size());
        const int synced=::fsync(fd),closed=::close(fd);
        if(n!=32||synced!=0||closed!=0||reset_identity()!=id)
            fail("reset marker write verification failed");
    }
    void erase_world_files(const std::string& expected){
        if(!valid_id(expected)||reset_identity()!=expected)fail("reset marker mismatch");
        if(world_absent())return;
        scan(false);
        // Identity is removed last. On an interrupted unlink, only an empty
        // directory may remain without its manifest; never erase foreign data.
        if(!exists(dir_+"/identity.pgim")){
            if(!scan(true))fail("unidentified nonempty reset directory");
        }else{
            if(identity()!=expected)fail("world identity changed");
            remove_file(dir_+"/world-a.pgid");
            remove_file(dir_+"/world-b.pgid");
            remove_file(dir_+"/identity.pgim");
            if(!scan(true))fail("reset directory not empty");
        }
        if(::rmdir(dir_.c_str())!=0)fail("reset directory removal failed");
        if(!world_absent())fail("reset directory still present");
    }
    void finish_reset(){
        if(!world_absent())fail("cannot clear marker before world is absent");
        remove_file(marker_);
    }
};
}
