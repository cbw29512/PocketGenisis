#include "../fixture.hpp"
#include "../profile.hpp"
#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <new>
#include <algorithm>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
using namespace genesis;

namespace {
constexpr unsigned samples=600;
#if defined(PG_DISPOSABLE_4096_PROBE)
constexpr unsigned populations[]={4096};
#elif defined(PG_DISPOSABLE_SCALE_PROBE)
constexpr unsigned populations[]={256,512,1024,2048};
#else
constexpr unsigned populations[]={2,32,64,128,256};
#endif
// Default initialization invokes element default constructors without an
// enormous aggregate brace expansion. Keep setup's code generation bounded.
__attribute__((noinline)) World* construct_world(void* raw) noexcept {
    return new(raw) World;
}
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
    Serial.printf("START initial=%u contact=%u\n",
        static_cast<unsigned>(total),static_cast<unsigned>(contact));
    Production p{};
    benchmark::Profile profile{};
    std::uint64_t elapsed=0,max_us=0;
    unsigned above_budget=0,above_second=0;
    std::size_t min_living=total;
    for(unsigned i=0;i<samples;++i) {
        std::uint64_t us=0;
        const bool ok=profile.step(w,p,us);
        if(!ok) {
            Serial.printf("ERROR rejected tick %u\n",i);
            return false;
        }
        elapsed+=us;
        max_us=std::max(max_us,us);
        above_budget+=us>200000;
        above_second+=us>1000000;
        min_living=std::min(min_living,w.descendant_count+2);
        if((i+1)%100==0) Serial.printf("PROGRESS ticks=%u/%u\n",i+1,samples);
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
    Serial.printf("LIMITS min_living=%u over_200ms=%u over_1000ms=%u stack_min_free_bytes=%u\n",
        static_cast<unsigned>(min_living),above_budget,above_second,
        static_cast<unsigned>(uxTaskGetStackHighWaterMark(nullptr)));
    profile.report(samples);
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
    auto* w=construct_world(raw);
    memory_report("allocated");
    bool ok=true;
    for(auto n:populations) {
        if(!measure(*w,n,false)||!measure(*w,n,true)) {ok=false;break;}
    }
    w->~World();
    heap_caps_free(raw);
    memory_report("released");
    Serial.println(ok?"BENCHMARK COMPLETE":"BENCHMARK FAILED");
}
void loop() {delay(1000);}
