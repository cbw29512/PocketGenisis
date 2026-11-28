#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <esp_timer.h>

// Standard DIS08070H only. Vendor SPI pin map, not the Advance board.
// Isolated test file; never format the card or touch civilization data.
namespace {
constexpr int cs=10,mosi=11,sck=12,miso=13;
constexpr char path[]="/sd/pg-probe-v1.bin";
constexpr unsigned blocks=80,block_size=4096;
std::array<uint8_t,block_size> buffer{};
void fill(unsigned block){
    for(unsigned i=0;i<buffer.size();++i)
        buffer[i]=static_cast<uint8_t>((block*31u+i*17u+(i>>8))&255u);
}
bool fail(const char* operation){
    Serial.printf("SD_ERROR %s errno=%d %s\n",operation,errno,strerror(errno));return false;
}
bool verify(){
    int fd=open(path,O_RDONLY);
    if(fd<0)return fail("open read");
    struct stat st{};
    bool ok=fstat(fd,&st)==0&&st.st_size==static_cast<off_t>(blocks*block_size);
    std::array<uint8_t,block_size> actual{};
    for(unsigned block=0;ok&&block<blocks;++block){
        fill(block);
        size_t pos=0;
        while(pos<actual.size()){
            auto n=read(fd,actual.data()+pos,actual.size()-pos);
            if(n<0&&errno==EINTR)continue;
            if(n<=0){ok=false;break;}
            pos+=static_cast<size_t>(n);
        }
        ok=ok&&actual==buffer;
        delay(1);
    }
    if(close(fd)!=0)ok=false;
    if(!ok)Serial.println("SD_ERROR length/content mismatch; existing file preserved");
    return ok;
}
bool create_probe(){
    // Exclusive creation avoids replacing an existing file, including a torn probe.
    int fd=open(path,O_WRONLY|O_CREAT|O_EXCL,0600);
    if(fd<0)return fail("exclusive create");
    bool ok=true;
    for(unsigned block=0;ok&&block<blocks;++block){
        fill(block);size_t pos=0;
        while(pos<buffer.size()){
            auto n=write(fd,buffer.data()+pos,buffer.size()-pos);
            if(n<0&&errno==EINTR)continue;
            if(n<=0){ok=fail("write");break;}
            pos+=static_cast<size_t>(n);
        }
        delay(1);
    }
    if(ok&&fsync(fd)!=0)ok=fail("fsync");
    if(close(fd)!=0)ok=fail("close");
    // Keep partial file for diagnosis; never silently remove or overwrite it.
    return ok;
}
}
void setup(){
    Serial.begin(115200);delay(5000);
    Serial.println("POCKET GENESIS DISPOSABLE SD PROBE; NO FORMAT");
    SPI.begin(sck,miso,mosi,cs);
    if(!SD.begin(cs,SPI,4000000,"/sd",5,false)||SD.cardType()==CARD_NONE){
        Serial.println("SD_ERROR mount/card unavailable");return;
    }
    Serial.printf("SD_MOUNT bytes=%llu\n",static_cast<unsigned long long>(SD.cardSize()));
    struct stat st{};
    const auto begin=esp_timer_get_time();
    if(stat(path,&st)==0){
        Serial.println("SD_EXISTING verifying prior boot file");
        if(!verify())return;
        Serial.printf("SD_RESTART_PASS bytes=%u us=%llu\n",blocks*block_size,
                      static_cast<unsigned long long>(esp_timer_get_time()-begin));
    }else{
        if(errno!=ENOENT){fail("stat probe");return;}
        if(!create_probe()||!verify())return;
        Serial.printf("SD_WRITE_PASS bytes=%u us=%llu\n",blocks*block_size,
                      static_cast<unsigned long long>(esp_timer_get_time()-begin));
    }
    Serial.println("SD_PROBE_COMPLETE");
}
void loop(){delay(1000);}
