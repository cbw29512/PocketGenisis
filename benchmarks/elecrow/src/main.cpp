#include "../fixture.hpp"
#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <new>
#include <algorithm>
using namespace genesis;

namespace {
constexpr unsigned samples=600;
void memory_report(const char* label) {
    constexpr auto internal=MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT;
    constexpr auto external=MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT;
    Serial.printf("MEM %s internal_free=%u internal_largest=%u psram_free=%u psram_largest=%u\n",
        label,static_cast<unsigned>(heap_caps_get_free_size(internal)),
        static_cast<unsigned>(heap_caps_get_largest_free_block(internal)),
        static_cast<unsigned>(heap_caps_get_free_size(external)),
        static_cast<unsigned>(heap_caps_get_largest_free_block(external)));
}
bool measure(World& w,std::size_t total,bool contact) {
    if(!benchmark::prepare(w,total,contact)) {
        Serial.println("ERROR invalid benchmark fixture");
        return false;
    }
    Production p{};
    std::uint64_t elapsed=0,max_us=0;
    for(unsigned i=0;i<samples;++i) {
        const auto start=esp_timer_get_time();
        const bool ok=step_world(w,p);
        const auto us=static_cast<std::uint64_t>(esp_timer_get_time()-start);
        if(!ok) {
            Serial.printf("ERROR rejected tick %u\n",i);
            return false;
        }
        elapsed+=us;
        max_us=std::max(max_us,us);
        // Yield outside measured time; no deliberate real-time wait in samples.
        delay(1);
    }
    if(!valid_population(w)) {
        Serial.println("ERROR invalid final state");
        return false;
    }
    Serial.printf("RESULT initial=%u final=%u contact=%u ticks=%u mean_us=%llu max_us=%llu deaths=%u\n",
        static_cast<unsigned>(total),static_cast<unsigned>(w.descendant_count+2),
        static_cast<unsigned>(contact),samples,
        static_cast<unsigned long long>(elapsed/samples),
        static_cast<unsigned long long>(max_us),static_cast<unsigned>(w.archive_count));
    memory_report("after_case");
    return true;
}
}
void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("POCKET GENESIS DISPOSABLE CPU/PSRAM BENCHMARK");
    Serial.printf("CPU_MHZ=%u FLASH_BYTES=%u PSRAM_BYTES=%u WORLD_BYTES=%u\n",
        static_cast<unsigned>(ESP.getCpuFreqMHz()),static_cast<unsigned>(ESP.getFlashChipSize()),
        static_cast<unsigned>(ESP.getPsramSize()),static_cast<unsigned>(sizeof(World)));
    memory_report("before");
    // Fail closed instead of silently moving the world to internal heap.
    void* raw=heap_caps_malloc(sizeof(World),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(!raw) {
        Serial.println("ERROR world allocation in PSRAM failed");
        return;
    }
    auto* w=new(raw) World{};
    memory_report("allocated");
    bool ok=true;
    for(auto n:{2u,32u,64u,128u,256u}) {
        if(!measure(*w,n,false)) {ok=false;break;}
    }
    if(ok) ok=measure(*w,256,true);
    w->~World();
    heap_caps_free(raw);
    memory_report("released");
    Serial.println(ok?"BENCHMARK COMPLETE":"BENCHMARK FAILED");
}
void loop() {delay(1000);}
