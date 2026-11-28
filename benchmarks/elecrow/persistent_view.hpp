#pragma once
#include "observer_inspection.hpp"
#include "observer_dirty_map.hpp"
// Read-only display facade. The storage runtime owns all simulation state.
namespace genesis::observer {
class PersistentView {
    GenesisPanel lcd_;
    Inspection inspection_;
    MapPainter map_;
    bool ready_{};
    std::uint8_t applied_brightness_{128};
    std::uint32_t last_frame_{};
    bool overview_was_open_{},citizen_was_open_{};
public:
    void save_status(const SaveStatus& saved) noexcept {inspection_.save_status(saved);}
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
        if(applied_brightness_!=inspection_.brightness()){
            applied_brightness_=inspection_.brightness();
            lcd_.setBrightness(applied_brightness_);
        }
        if(!force&&!touched&&std::uint32_t(now-last_frame_)<1000)return;
        const auto start=esp_timer_get_time();
        const bool overview=inspection_.overview_open();
        if(overview){
            if(!overview_was_open_||force)inspection_.invalidate_summary();
            // Never draw the map under an open settings panel.
            inspection_.draw_overlay(lcd_,state);
        }else{
            if(force||overview_was_open_||(citizen_was_open_&&!inspection_.citizen_open()))map_.invalidate();
            map_.draw(lcd_,state);
            inspection_.draw_overlay(lcd_,state);
        }
        overview_was_open_=overview;
        citizen_was_open_=inspection_.citizen_open();
        last_frame_=millis();
        if(force)Serial.printf("WORLD_DISPLAY_FRAME tick=%llu us=%llu\n",
            static_cast<unsigned long long>(state.world.tick),
            static_cast<unsigned long long>(esp_timer_get_time()-start));
    }
};
}
