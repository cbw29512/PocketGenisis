#pragma once
#include "observer_map.hpp"
#include "panel_rgb.hpp"
namespace genesis::observer {
// Direct RGB framebuffer renderer: no separate full-screen sprite allocation.
// A tile occupies exactly eight by eight display pixels. All drawing reads
// const authoritative state, including the live citizen positions.
inline void citizen(GenesisPanel& lcd,const Citizen& c,bool founder) noexcept {
    if(c.x>=width||c.y>=height) return;
    const int x=c.x*tile_px,y=c.y*tile_px;
    lcd.fillRect(x+2,y+1,4,6,rgb(20,26,25));
    lcd.fillRect(x+3,y+1,2,2,rgb(255,222,168));
    lcd.fillRect(x+2,y+3,4,3,founder?rgb(255,205,45):rgb(242,240,216));
    lcd.drawPixel(x+2,y+6,rgb(30,30,30));
    lcd.drawPixel(x+5,y+6,rgb(30,30,30));
}
inline void draw(GenesisPanel& lcd,const integrated::State& s) noexcept {
    lcd.startWrite();
    for(std::size_t i=0;i<s.world.tiles.size();++i) {
        const int x=int(i%width)*tile_px,y=int(i/width)*tile_px;
        const auto t=view(s,i);
        lcd.fillRect(x,y,tile_px,tile_px,t.ground);
        if(t.tree) {
            lcd.fillRect(x+3,y+5,2,2,rgb(96,72,42));
            lcd.fillRect(x+2,y+1,4,4,rgb(25,68,36));
        }
        if(t.fish) lcd.fillRect(x+1,y+5,3,1,rgb(157,222,235));
        if(t.prey) lcd.fillRect(x+4,y+5,3,2,rgb(232,212,166));
        if(t.predator) lcd.fillRect(x+5,y+1,2,2,rgb(192,65,49));
    }
    for(unsigned k=0;k<construction::site_count;++k) {
        const auto& site=s.sites.sites[k];
        if(site.stage==construction::Stage::empty||site.tile>=s.world.tiles.size()) continue;
        const int x=(site.tile%width)*tile_px,y=(site.tile/width)*tile_px;
        const auto color=site_color(construction::kind_at(k));
        lcd.drawRect(x+1,y+1,6,6,color);
        if(site.stage==construction::Stage::complete) lcd.fillRect(x+2,y+2,4,4,color);
    }
    for(std::size_t i=0;i<s.world.descendant_count;++i) citizen(lcd,s.world.descendants[i],false);
    for(const auto& founder:s.world.founders) citizen(lcd,founder,true);
#ifdef PG_OBSERVER_TOUCH_PROBE
    // Oversized contrast rings deliberately extend beyond the tiny sprite.
    // Clamp the label inside the display; keep actual citizen coordinates.
    for(const auto& c:s.world.founders) {
        const int x=c.x*tile_px+4,y=c.y*tile_px+4;
        lcd.drawCircle(x,y,10,rgb(0,0,0));
        lcd.drawCircle(x,y,9,rgb(255,255,255));
        lcd.drawCircle(x,y,8,rgb(255,205,45));
        const int lx=std::clamp(x+11,0,770),ly=std::clamp(y-12,0,465);
        lcd.fillRect(lx,ly,24,13,rgb(0,0,0));
        lcd.setTextSize(1);
        lcd.setTextColor(rgb(255,230,90),rgb(0,0,0));
        lcd.setCursor(lx+2,ly+2);
        lcd.printf("F%u",unsigned(c.id));
    }
#endif
    lcd.endWrite();
}
}
