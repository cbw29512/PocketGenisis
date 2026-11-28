#pragma once
#include <cstdint>
namespace genesis::observer {
// Presentation and pacing are separate from the deterministic world.
inline constexpr unsigned rates[]={1,3,12,24,48};
inline constexpr const char* rate_labels[]={"12h/year","4h/year","1h/year","30m/year","15m/year"};
enum class StatsPage : std::uint8_t { world, people, deaths, progress, settings };
struct DisplaySettings {
    StatsPage page{StatsPage::world};
    unsigned death_page{},death_page_count{1};
    void sync_deaths(unsigned pages) noexcept {
        death_page_count=pages?pages:1;
        if(death_page>=death_page_count)death_page=death_page_count-1;
    }
    std::uint8_t brightness{128},speed{2},reset_stage{};
    bool overview{},reset_requested{};
    unsigned ticks_per_second() const noexcept {return rates[speed];}
    bool take_reset() noexcept {const bool requested=reset_requested;reset_requested=false;return requested;}
    bool touch(int x,int y) noexcept {
        if(!overview)return false;
        if(reset_stage){
            const bool left=y>=376&&y<420&&x>=48&&x<350;
            const bool right=y>=376&&y<420&&x>=450&&x<752;
            // Final erase moves to the opposite side: double-tap cannot erase.
            if((reset_stage==1&&left)||(reset_stage==2&&right)){reset_stage=0;return true;}
            if(reset_stage==1&&right){reset_stage=2;return true;}
            if(reset_stage==2&&left){reset_requested=true;reset_stage=0;return true;}
            return false;
        }
        if(y>=44&&y<88&&x>=400&&x<456){if(speed>0)--speed;return true;}
        if(y>=44&&y<88&&x>=696&&x<752){if(speed<4)++speed;return true;}
        // Six large navigation targets. Reset is available on Settings only.
        if(y>=376&&y<420&&x>=48&&x<752){
            const unsigned col=unsigned(x-48)/118;
            if(col>=6||unsigned(x-48)%118>=114)return false;
            if(col==5){overview=false;page=StatsPage::world;death_page=0;}
            else {page=static_cast<StatsPage>(col);}
            return true;
        }
        if(page==StatsPage::deaths&&y>=264&&y<300){
            if(x>=420&&x<560){if(death_page)--death_page;return true;}
            if(x>=600&&x<752){if(death_page+1<death_page_count)++death_page;return true;}
        }
        if(page!=StatsPage::settings)return false;
        if(x>=48&&x<360&&y>=264&&y<296){reset_stage=1;return true;}
        if(x>=48&&x<216&&y>=198&&y<242)brightness=64;
        else if(x>=232&&x<400&&y>=198&&y<242)brightness=128;
        else if(x>=416&&x<584&&y>=198&&y<242)brightness=255;
        else return false;
        return true;
    }
};
struct SaveStatus {
    std::uint64_t generation{},tick{},card_bytes{};
    int slot{-1};
};
// Bounded scheduler: no fast-forwarding world state or unbounded catch-up.
// A stall reanchors the wall-clock deadline, never skips simulation ticks.
class TickPacer {
    std::uint64_t next_{};
    unsigned rate_{1};
public:
    void arm(std::uint64_t now,unsigned rate) noexcept {
        rate_=rate?rate:1;next_=now+1000000/rate_;
    }
    bool due(std::uint64_t now,unsigned rate) noexcept {
        if(rate!=rate_){arm(now,rate);return false;}
        if(now<next_)return false;
        if(now-next_>1000000)next_=now;
        next_+=1000000/rate_;
        return true;
    }
};
}
