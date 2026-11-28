#pragma once
#include "observer_render.hpp"
#include "observer_settings.hpp"
namespace genesis::observer {
// Derived counters only. No RNG, state mutation or checkpoint format change.
struct WorldSummary {
    std::uint64_t tick{},food{},water{},wood{},stone{},fish{},wild{};
    std::size_t living{},deaths{},pregnancies{},discoveries{},buildings{};
    std::array<unsigned,5> stages{};
};
inline WorldSummary summarize(const integrated::State& s) noexcept {
    WorldSummary out{};
    out.tick=s.world.tick;
    out.living=s.world.founders.size()+s.world.descendant_count;
    out.deaths=s.world.archive_count;
    out.pregnancies=s.world.pregnancy_count;
    const auto count=[&](const Citizen& c){++out.stages[static_cast<unsigned>(life_stage(c,out.tick))];};
    for(const auto& c:s.world.founders)count(c);
    for(std::size_t i=0;i<s.world.descendant_count;++i)count(s.world.descendants[i]);
    for(const auto& tile:s.world.tiles){
        out.food+=tile.food;out.water+=tile.water;out.wood+=tile.wood;out.stone+=tile.stone;
    }
    for(const auto& cell:s.fish.cells)out.fish+=cell.fish;
    out.wild=wildlife::population(s.wild);
    for(const auto& record:s.knowledge.first)if(record.founder)++out.discoveries;
    for(const auto& site:s.sites.sites)if(site.stage==construction::Stage::complete)++out.buildings;
    return out;
}
// A compact globe, with a larger invisible touch target. Stippled
// shading leaves the actual map visible between pixels (RGB565, no alpha).
inline constexpr int globe_left=744,globe_top=0,globe_size=56;
inline bool globe_hit(int x,int y) noexcept {
    return x>=globe_left&&x<globe_left+globe_size&&
           y>=globe_top&&y<globe_top+globe_size;
}
inline void draw_world_button(GenesisPanel& lcd) {
    constexpr int cx=772,cy=28,r=21;
    const auto line=rgb(220,236,229),shade=rgb(22,42,51);
    for(int y=-r;y<=r;++y)for(int x=-r;x<=r;++x){
        const int d=x*x+y*y;
        if(d>r*r)continue;
        const bool rim=d>=(r-2)*(r-2);
        const bool meridian=(4*x*x+y*y>=r*r-40&&4*x*x+y*y<=r*r+40);
        const bool latitude=(y==0||y==-10||y==10);
        // Half-tone strokes and quarter-density shading retain map detail.
        if((rim||meridian||latitude)&&((x+y)&1)==0)
            lcd.drawPixel(cx+x,cy+y,line);
        else if((x&1)==0&&(y&1)==0)
            lcd.drawPixel(cx+x,cy+y,shade);
    }
}
inline void draw_summary(GenesisPanel& lcd,const WorldSummary& s,
                         const DisplaySettings& settings,const SaveStatus& saved) {
    const auto bg=rgb(12,22,30),fg=rgb(245,245,235),muted=rgb(151,181,190);
    lcd.fillRect(30,30,740,420,bg);lcd.drawRect(30,30,740,420,fg);
    lcd.setTextColor(fg,bg);lcd.setTextSize(3);
    lcd.setCursor(48,48);lcd.print("World / Settings");
    lcd.setTextSize(2);
    lcd.setCursor(48,90);lcd.printf("Year %llu | Tick %llu",(unsigned long long)(s.tick/ticks_per_year+1),(unsigned long long)s.tick);
    lcd.setCursor(48,126);lcd.printf("Living: %u",unsigned(s.living));
    lcd.setCursor(48,150);lcd.printf("Pregnancies: %u",unsigned(s.pregnancies));
    lcd.setCursor(48,174);lcd.printf("Recorded deaths: %u",unsigned(s.deaths));
    lcd.setCursor(48,198);lcd.printf("Infants: %u  Children: %u",s.stages[0],s.stages[1]);
    lcd.setCursor(48,222);lcd.printf("Teens: %u  Adults: %u",s.stages[2],s.stages[3]);
    lcd.setCursor(48,246);lcd.printf("Elders: %u",s.stages[4]);
    lcd.setCursor(420,126);lcd.printf("Food: %llu",(unsigned long long)s.food);
    lcd.setCursor(420,150);lcd.printf("Water: %llu",(unsigned long long)s.water);
    lcd.setCursor(420,174);lcd.printf("Wood: %llu  Stone: %llu",(unsigned long long)s.wood,(unsigned long long)s.stone);
    lcd.setCursor(420,198);lcd.printf("Wild animals: %llu",(unsigned long long)s.wild);
    lcd.setCursor(420,222);lcd.printf("Fish: %llu",(unsigned long long)s.fish);
    lcd.setCursor(420,246);lcd.printf("Discoveries: %u",unsigned(s.discoveries));
    lcd.setCursor(420,270);lcd.printf("Buildings: %u",unsigned(s.buildings));
    lcd.drawFastHLine(48,298,704,muted);
    lcd.setCursor(48,310);lcd.printf("SD card: %.1f GB",(double)saved.card_bytes/1000000000.0);
    lcd.setCursor(48,334);
    if(saved.generation)lcd.printf("Verified save #%llu | Tick %llu | Slot %c",
        (unsigned long long)saved.generation,(unsigned long long)saved.tick,saved.slot==0?'A':'B');
    else lcd.print("No verified checkpoint");
    const auto button=[&](int x,int w,const char* label,bool active){
        lcd.fillRoundRect(x,376,w,44,5,active?rgb(58,105,80):rgb(36,53,62));
        lcd.setTextColor(fg);lcd.setCursor(x+12,390);lcd.print(label);
    };
    button(48,168,"Dim",settings.brightness==64);
    button(232,168,"Normal",settings.brightness==128);
    button(416,168,"Bright",settings.brightness==255);
    button(600,152,"Close",false);
}
}
