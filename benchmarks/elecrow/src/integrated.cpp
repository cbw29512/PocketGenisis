// Only the dedicated environment compiles this disposable test.
#ifdef PG_INTEGRATED_TARGET_PROBE
#include "../../../core/integrated_world.hpp"
#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <new>
#include <algorithm>

namespace {
using namespace genesis;
// Preserve the verified short environment and select a full annual run
// only in the explicitly named annual environment. Rules are unchanged.
#ifdef PG_INTEGRATED_ANNUAL_PROBE
constexpr unsigned samples=43200;
constexpr unsigned report_interval=3600;
#else
constexpr unsigned samples=3600;
constexpr unsigned report_interval=600;
#endif
constexpr std::uint64_t seed=42;
constexpr std::uint64_t budget_us=200000;
constexpr auto external=MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT;
constexpr auto internal=MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT;
void memory(const char* label) noexcept {
    Serial.printf("MEM %s internal_free=%u internal_largest=%u psram_free=%u psram_largest=%u stack_free=%u\n",
        label,unsigned(heap_caps_get_free_size(internal)),
        unsigned(heap_caps_get_largest_free_block(internal)),
        unsigned(heap_caps_get_free_size(external)),
        unsigned(heap_caps_get_largest_free_block(external)),
        unsigned(uxTaskGetStackHighWaterMark(nullptr)));
}
__attribute__((noinline)) integrated::Simulation* construct(void* raw) noexcept {
    return new(raw) integrated::Simulation;
}
bool measure(integrated::Simulation& sim) noexcept {
    const auto start=esp_timer_get_time();
    if(!sim.create(seed)) {
        Serial.println("ERROR rejected disposable creation");
        return false;
    }
    const auto& s=sim.state();
    Serial.printf("FOUNDING requested=%llu accepted=%llu attempts=%u terrain_hash=%llu creation_us=%llu\n",
        static_cast<unsigned long long>(seed),static_cast<unsigned long long>(s.world.seed),
        s.founding.attempts,static_cast<unsigned long long>(terrain_probe::fingerprint(s.terrain)),
        static_cast<unsigned long long>(esp_timer_get_time()-start));
    if(sim.create(seed+1)||!integrated::valid(s)) {
        Serial.println("ERROR creation guard or state invariant");
        return false;
    }
    Serial.printf("BASELINE living=%u fish=%llu wild=%llu\n",
        unsigned(s.world.descendant_count+2),
        static_cast<unsigned long long>(freshwater::population(s.fish)),
        static_cast<unsigned long long>(wildlife::population(s.wild)));
    std::uint64_t sum=0,peak=0,peak_tick=0;
    unsigned over_budget=0,over_second=0;
    for(unsigned n=0;n<samples;++n) {
        const auto before=esp_timer_get_time();
        if(!sim.step(discovery::probe_cadence)) {
            Serial.printf("ERROR step rejected at tick=%u\n",n+1);
            return false;
        }
        const auto elapsed=static_cast<std::uint64_t>(esp_timer_get_time()-before);
        sum+=elapsed;
        if(elapsed>peak) {peak=elapsed;peak_tick=n+1;}
        over_budget+=elapsed>budget_us;over_second+=elapsed>1000000;
        // Boundary validation outside timed ticks, including ecology ledgers.
        if((n+1)%600==0) {
            if(!integrated::valid(s)||s.world.tick!=n+1) {
                Serial.printf("ERROR invalid integrated state at tick=%u\n",n+1);
                return false;
            }
            if((n+1)%report_interval==0) {
                Serial.printf("PROGRESS ticks=%u/%u season=%u living=%u fish=%llu wild=%llu crops=%llu livestock=%u\n",
                    n+1,samples,season(s.world),unsigned(s.world.descendant_count+2),
                    static_cast<unsigned long long>(freshwater::population(s.fish)),
                    static_cast<unsigned long long>(wildlife::population(s.wild)),
                    static_cast<unsigned long long>(multi_field::harvests(s.farms)),
                    livestock::population(s.animals));
                memory("sample");
            }
        }
        delay(1); // yield outside measured interval
    }
    Serial.printf("INTEGRATED_RESULT ticks=%u mean_us=%llu max_us=%llu over_200ms=%u over_1000ms=%u discoveries=%u crops=%llu livestock=%u\n",
        samples,static_cast<unsigned long long>(sum/samples),static_cast<unsigned long long>(peak),
        over_budget,over_second,discovery::discovered(s.knowledge),
        static_cast<unsigned long long>(multi_field::harvests(s.farms)),livestock::population(s.animals));
    // Observe actual transactions. Do not fabricate positive milestones
    // or force the annual outcome to match an earlier host run.
    Serial.printf("LIFECYCLE issued_descendants=%llu living=%u archived=%u pregnancies=%u lost_gestations=%llu sites=%u\n",
        static_cast<unsigned long long>(s.world.next_citizen_id-3),
        unsigned(s.world.descendant_count+2),unsigned(s.world.archive_count),
        unsigned(s.world.pregnancy_count),static_cast<unsigned long long>(s.world.lost_gestations),
        construction::completed(s.sites));
    Serial.printf("FISH initial=%llu final=%llu births=%llu deaths=%llu caught=%llu migrated=%llu\n",
        static_cast<unsigned long long>(s.fish.initial),
        static_cast<unsigned long long>(freshwater::population(s.fish)),
        static_cast<unsigned long long>(s.fish.births),static_cast<unsigned long long>(s.fish.deaths),
        static_cast<unsigned long long>(s.fish.caught),static_cast<unsigned long long>(s.fish.migrated));
    Serial.printf("WILDLIFE initial=%llu final=%llu births=%llu deaths=%llu captured=%llu hunted=%llu meals=%llu\n",
        static_cast<unsigned long long>(s.initial_wild),
        static_cast<unsigned long long>(wildlife::population(s.wild)),
        static_cast<unsigned long long>(s.ecology.births),static_cast<unsigned long long>(s.ecology.deaths),
        static_cast<unsigned long long>(livestock::captures(s.animals)),
        static_cast<unsigned long long>(s.ecology.hunted),static_cast<unsigned long long>(s.ecology.meals));
    Serial.printf("FARM harvests=%llu manure_produced=%llu manure_applied=%llu fodder=%u\n",
        static_cast<unsigned long long>(multi_field::harvests(s.farms)),
        static_cast<unsigned long long>(s.animals.organic.produced),
        static_cast<unsigned long long>(s.animals.organic.applied),unsigned(s.animals.fodder.balance));
    const bool consistent=integrated::valid(s);
    Serial.printf("ANNUAL_EVIDENCE peak_tick=%llu valid=%u\n",
        static_cast<unsigned long long>(peak_tick),unsigned(consistent));
    return !over_budget&&consistent;
}
}
void setup() {
    Serial.begin(115200);
    // Allow the owner to open the serial monitor after a USB upload.
    // This delay is outside all measured simulation ticks.
    delay(10000);
    Serial.printf("POCKET GENESIS DISPOSABLE INTEGRATED CPU/PSRAM PROBE ticks=%u\n",samples);
    Serial.printf("CPU_MHZ=%u FLASH_BYTES=%u PSRAM_BYTES=%u SIM_BYTES=%u STATE_BYTES=%u\n",
        unsigned(ESP.getCpuFreqMHz()),unsigned(ESP.getFlashChipSize()),unsigned(ESP.getPsramSize()),
        unsigned(sizeof(integrated::Simulation)),unsigned(sizeof(integrated::State)));
    memory("before");
    // Reject unsupported memory rather than fall back to internal heap.
    void* raw=heap_caps_malloc(sizeof(integrated::Simulation),external);
    if(!raw) {Serial.println("ERROR integrated PSRAM allocation failed");return;}
    auto* sim=construct(raw);
    memory("allocated");
    const bool ok=measure(*sim);
    sim->~Simulation();heap_caps_free(raw);
    memory("released");
    Serial.println(ok?"INTEGRATED BENCHMARK COMPLETE":"INTEGRATED BENCHMARK FAILED");
}
void loop() {delay(1000);}
#endif
