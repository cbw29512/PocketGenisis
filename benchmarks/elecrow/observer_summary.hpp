#pragma once
#include "observer_render.hpp"
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
inline void draw_world_button(GenesisPanel& lcd) {
    lcd.fillRoundRect(680,4,116,36,5,rgb(17,28,35));
    lcd.setTextColor(rgb(245,245,235));lcd.setTextSize(2);
    lcd.setCursor(695,13);lcd.print("WORLD");
}
inline void draw_summary(GenesisPanel& lcd,const WorldSummary& s) {
    const auto bg=rgb(12,22,30),fg=rgb(245,245,235);
    lcd.fillRect(30,30,740,420,bg);lcd.drawRect(30,30,740,420,fg);
    lcd.setTextColor(fg,bg);lcd.setTextSize(3);
    lcd.setCursor(48,48);lcd.print("World overview");
    lcd.setTextSize(2);
    lcd.setCursor(48,90);lcd.printf("Year %llu | Tick %llu",static_cast<unsigned long long>(s.tick/ticks_per_year+1),static_cast<unsigned long long>(s.tick));
    lcd.setCursor(48,130);lcd.printf("Living: %u  Pregnancies: %u",unsigned(s.living),unsigned(s.pregnancies));
    lcd.setCursor(48,160);lcd.printf("Recorded deaths: %u",unsigned(s.deaths));
    lcd.setCursor(48,190);lcd.printf("Infants %u  Children %u  Teens %u",s.stages[0],s.stages[1],s.stages[2]);
    lcd.setCursor(48,220);lcd.printf("Adults %u  Elders %u",s.stages[3],s.stages[4]);
    lcd.setCursor(48,260);lcd.printf("Food %llu  Water %llu",(unsigned long long)s.food,(unsigned long long)s.water);
    lcd.setCursor(48,290);lcd.printf("Wood %llu  Stone %llu",(unsigned long long)s.wood,(unsigned long long)s.stone);
    lcd.setCursor(48,330);lcd.printf("Wild animals %llu  Fish %llu",(unsigned long long)s.wild,(unsigned long long)s.fish);
    lcd.setCursor(48,360);lcd.printf("Discoveries %u  Buildings %u",unsigned(s.discoveries),unsigned(s.buildings));
    lcd.setCursor(48,420);lcd.print("Tap to return to world");
}
}
