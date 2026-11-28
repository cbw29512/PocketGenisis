#pragma once
#include "observer_render.hpp"
#ifdef PG_PERSISTENT_OBSERVER
#include "observer_summary.hpp"
#endif
#include <lgfx/v1/touch/Touch_GT911.hpp>
namespace genesis::observer {
// Only observer state: selected stable ID and last rendered hit targets.
// Never modify a Citizen or call a simulation tick from touch handling.
class Inspection {
    lgfx::Touch_GT911 touch_;
    struct Marker {std::uint64_t id;int x,y;};
    std::array<Marker,descendant_capacity+2> marks_{};
    std::size_t count_{};
    std::uint64_t selected_{};
    bool ready_{},pressed_{};
#ifdef PG_PERSISTENT_OBSERVER
    DisplaySettings settings_{};
    SaveStatus saved_{};
#endif
    std::uint32_t last_poll_{};
public:
#ifdef PG_PERSISTENT_OBSERVER
    void save_status(const SaveStatus& saved) noexcept {saved_=saved;}
    std::uint8_t brightness() const noexcept {return settings_.brightness;}
#endif
    void begin() noexcept {
        auto cfg=touch_.config();
        cfg.i2c_port=1;cfg.pin_sda=19;cfg.pin_scl=20;
        cfg.i2c_addr=0x14;cfg.freq=100000;
        cfg.pin_int=-1;cfg.pin_rst=-1;cfg.bus_shared=false;
        cfg.x_min=0;cfg.x_max=799;cfg.y_min=0;cfg.y_max=479;
        touch_.config(cfg);
        // Pinned driver tries 0x14 and 0x5D itself. No external reset,
        // expander changes, touch-count reconfiguration or SD access.
        ready_=touch_.init();
        Serial.printf("TOUCH ready=%u addr=0x%02x no_reset=1\n",
            unsigned(ready_),unsigned(touch_.config().i2c_addr));
    }
    bool poll(std::uint32_t now) noexcept {
        if(!ready_||std::uint32_t(now-last_poll_)<50) return false;
        last_poll_=now;
        lgfx::touch_point_t p{};
        const bool down=touch_.getTouchRaw(&p,1)!=0;
        if(!down) {pressed_=false;return false;}
        if(pressed_) return false;
        pressed_=true;
        Serial.printf("TOUCH x=%d y=%d\n",int(p.x),int(p.y));
        if(p.x<0||p.x>=800||p.y<0||p.y>=480) {
            Serial.println("TOUCH outside native bounds");return false;
        }
#ifdef PG_PERSISTENT_OBSERVER
        if(settings_.overview)return settings_.touch(p.x,p.y);
        if(globe_hit(p.x,p.y)){settings_.overview=true;selected_=0;return true;}
#endif
        // Outside the small globe target, map positions remain inspectable
        // unless the temporary citizen panel covers them.
        if(selected_&&p.x<300&&p.y<176) selected_=0;
        else {
            selected_=0;int best=25*25;
            for(std::size_t i=0;i<count_;++i) {
                const auto& m=marks_[i];
                const int dx=p.x-m.x,dy=p.y-m.y,d=dx*dx+dy*dy;
                if(d<best) {best=d;selected_=m.id;}
            }
        }
        Serial.printf("INSPECT selected=%llu\n",static_cast<unsigned long long>(selected_));
        return true;
    }
    void draw_overlay(GenesisPanel& lcd,const integrated::State& s) noexcept {
        count_=0;
        const auto add=[&](const Citizen& c) noexcept {
            if(count_<marks_.size()) marks_[count_++]={c.id,int(c.x)*8+4,int(c.y)*8+4};
        };
        for(const auto& c:s.world.founders) add(c);
        for(std::size_t i=0;i<s.world.descendant_count;++i) add(s.world.descendants[i]);
#ifdef PG_PERSISTENT_OBSERVER
        if(settings_.overview){draw_summary(lcd,summarize(s),settings_,saved_);return;}
        draw_world_button(lcd);
#endif
        // An unselected observer draws nothing over the civilization.
        if(!selected_) return;
        const auto black=rgb(12,17,22),white=rgb(245,245,235);
        lcd.startWrite();
        lcd.setTextSize(2);lcd.setTextColor(white,black);
        if(selected_) {
            const auto* c=live_citizen(s.world,selected_);
            lcd.fillRect(0,0,300,176,black);lcd.drawRect(0,0,300,176,white);
            lcd.setCursor(8,8);lcd.setTextColor(white,black);
            if(!c) lcd.print("No longer living");
            else {
                if(c->id==1||c->id==2)
                    lcd.printf("Founder %llu",static_cast<unsigned long long>(c->id));
                else lcd.printf("Pixel %llu",static_cast<unsigned long long>(c->id));
                lcd.setTextSize(1);
                lcd.setCursor(8,34);lcd.printf("Position %u,%u  Health %u",unsigned(c->x),unsigned(c->y),unsigned(c->health));
                lcd.setCursor(8,50);lcd.printf("Hunger %u  Thirst %u  Fatigue %u",unsigned(c->hunger),unsigned(c->thirst),unsigned(c->fatigue));
                lcd.setCursor(8,66);lcd.printf("Life stage %u  Activity %u",unsigned(life_stage(*c,s.world.tick)),unsigned(c->action));
                lcd.setCursor(8,82);lcd.printf("Parents %llu / %llu",static_cast<unsigned long long>(c->parent1),static_cast<unsigned long long>(c->parent2));
                lcd.setCursor(8,98);lcd.printf("Purpose %u  Reproductive drive %u",unsigned(c->purpose),unsigned(c->reproductive_drive));
            }
            lcd.setTextSize(1);lcd.setCursor(8,152);lcd.print("Tap this panel to close");
        }
        lcd.endWrite();
    }
};
}
