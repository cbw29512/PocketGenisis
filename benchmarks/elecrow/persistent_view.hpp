#pragma once
#include "observer_inspection.hpp"
// Read-only display facade. The storage runtime owns all simulation state.
namespace genesis::observer {
class PersistentView {
    GenesisPanel lcd_;
    Inspection inspection_;
    bool ready_{};
    std::uint32_t last_frame_{};
public:
    void begin(){
        if(!lcd_.init())throw std::runtime_error("LCD initialization failed");
        lcd_.setBrightness(128);
        if(lcd_.width()!=800||lcd_.height()!=480)
            throw std::runtime_error("unexpected display dimensions");
        ready_=true;
        status("Restoring saved civilization...");
        inspection_.begin();
        Serial.println("WORLD_DISPLAY_READY 800x480");
    }
    void status(const char* message){
        if(!ready_)return;
        lcd_.fillScreen(rgb(12,17,22));
        lcd_.setTextSize(2);lcd_.setTextColor(rgb(245,245,235));
        lcd_.setCursor(16,16);lcd_.println("Pocket Genesis");
        lcd_.setCursor(16,56);lcd_.println(message);
    }
    void update(const integrated::State& state,bool force=false){
        if(!ready_)return;
        const auto now=millis();
        const bool touched=inspection_.poll(now);
        if(!force&&!touched&&std::uint32_t(now-last_frame_)<1000)return;
        const auto start=esp_timer_get_time();
        draw(lcd_,state);
        inspection_.draw_overlay(lcd_,state);
        last_frame_=millis();
        if(force)Serial.printf("WORLD_DISPLAY_FRAME tick=%llu us=%llu\n",
            static_cast<unsigned long long>(state.world.tick),
            static_cast<unsigned long long>(esp_timer_get_time()-start));
    }
};
}
