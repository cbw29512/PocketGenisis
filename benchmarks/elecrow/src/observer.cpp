#ifdef PG_OBSERVER_RGB_PROBE
#include "../observer_render.hpp"
#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <new>
#ifdef PG_OBSERVER_TOUCH_PROBE
#include "../observer_inspection.hpp"
#endif

namespace {
using namespace genesis;
GenesisPanel lcd;
#ifdef PG_OBSERVER_TOUCH_PROBE
observer::Inspection inspection;
#endif
integrated::Simulation* simulation=nullptr;
// Explicit disposable acceleration, not production clock or user control.
constexpr std::uint32_t step_ms=200,frame_ms=1000;
std::uint32_t last_step=0,last_frame=0;
std::uint64_t tick_peak=0,frame_peak=0;
unsigned frames=0;
void memory(const char* label) noexcept {
    Serial.printf("MEM %s internal=%u psram=%u stack=%u\n",label,
        unsigned(heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)),
        unsigned(heap_caps_get_free_size(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT)),
        unsigned(uxTaskGetStackHighWaterMark(nullptr)));
}
__attribute__((noinline)) integrated::Simulation* construct(void* raw) noexcept {
    return new(raw) integrated::Simulation;
}
void halt(const char* why) noexcept {
    Serial.printf("ERROR %s\n",why);
    if(simulation) {
        simulation->~Simulation();
        heap_caps_free(simulation);
        simulation=nullptr;
    }
}
void frame() noexcept {
    const auto start=esp_timer_get_time();
    observer::draw(lcd,simulation->state());
#ifdef PG_OBSERVER_TOUCH_PROBE
    inspection.draw_overlay(lcd,simulation->state());
#endif
    const auto us=std::uint64_t(esp_timer_get_time()-start);
    if(us>frame_peak) frame_peak=us;
    ++frames;
}
}
void setup() {
    Serial.begin(115200);
    delay(10000);
    Serial.println("POCKET GENESIS DISPOSABLE RGB OBSERVER - NO SAVE");
    memory("before");
    if(!lcd.init()) {halt("LCD initialization failed");return;}
    lcd.setBrightness(128);
    if(lcd.width()!=800||lcd.height()!=480) {halt("unexpected LCD dimensions");return;}
    // Color/geometry check before rendering; RGB only, no touch I2C or SD.
    lcd.fillRect(0,0,266,480,observer::rgb(255,0,0));
    lcd.fillRect(266,0,267,480,observer::rgb(0,255,0));
    lcd.fillRect(533,0,267,480,observer::rgb(0,0,255));
    Serial.println("RGB TEST: RED GREEN BLUE");
    delay(2000);
    memory("lcd_ready");
#ifdef PG_OBSERVER_TOUCH_PROBE
    inspection.begin();
#endif
    void* raw=heap_caps_malloc(sizeof(integrated::Simulation),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(!raw) {halt("PSRAM allocation failed");return;}
    simulation=construct(raw);
    if(!simulation->create(42)) {halt("world creation refused");return;}
    Serial.printf("FOUNDING seed=%llu terrain_hash=%llu\n",
        static_cast<unsigned long long>(simulation->state().world.seed),
        static_cast<unsigned long long>(terrain_probe::fingerprint(simulation->state().terrain)));
    frame();
    memory("first_frame");
    last_step=last_frame=millis();
}
void loop() {
    if(!simulation) {delay(1000);return;}
    const auto now=millis();
    if(std::uint32_t(now-last_step)>=step_ms) {
        last_step=now; // Never skip or fast-forward authoritative ticks.
        const auto start=esp_timer_get_time();
        if(!simulation->step(discovery::probe_cadence)) {halt("tick rejected");return;}
        const auto elapsed=std::uint64_t(esp_timer_get_time()-start);
        if(elapsed>tick_peak) tick_peak=elapsed;
        const auto& s=simulation->state();
        if(s.world.tick%600==0) {
            if(!integrated::valid(s)) {halt("state invariant failed");return;}
            Serial.printf("OBSERVER ticks=%llu living=%u frames=%u tick_peak_us=%llu frame_peak_us=%llu\n",
                static_cast<unsigned long long>(s.world.tick),unsigned(s.world.descendant_count+2),
                frames,static_cast<unsigned long long>(tick_peak),static_cast<unsigned long long>(frame_peak));
            for(const auto& c:s.world.founders)
                Serial.printf("FOUNDER id=%llu x=%u y=%u health=%u\n",
                    static_cast<unsigned long long>(c.id),unsigned(c.x),unsigned(c.y),unsigned(c.health));
            memory("sample");
        }
        // Finish at the already tested annual boundary; leave final image.
        if(s.world.tick>=ticks_per_year) {
            frame();Serial.println("OBSERVER ANNUAL COMPLETE");
            simulation->~Simulation();heap_caps_free(simulation);simulation=nullptr;return;
        }
    }
#ifdef PG_OBSERVER_TOUCH_PROBE
    if(inspection.poll(millis())) {frame();last_frame=millis();}
#endif
    if(std::uint32_t(millis()-last_frame)>=frame_ms) {
        last_frame=millis();
        frame();
    }
    delay(1);
}
#endif
