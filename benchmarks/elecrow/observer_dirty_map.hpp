#pragma once
#include "observer_render.hpp"
namespace genesis::observer {
// Visual cache only: no simulation copies, RNG calls or save changes.
// Redraw terrain whose appearance changed, plus old/new sprite footprints.
// Restoring those footprints before drawing sprites prevents movement trails.
class MapPainter {
    static constexpr std::size_t count=std::size_t(width)*height;
    static constexpr std::uint32_t occupied=1u<<20;
    std::array<std::uint32_t,count> previous_{};
    bool initialized_{};
    static std::uint32_t signature(const TileView& t) noexcept {
        return std::uint32_t(t.ground)|(std::uint32_t(t.tree)<<16)|
            (std::uint32_t(t.fish)<<17)|(std::uint32_t(t.prey)<<18)|
            (std::uint32_t(t.predator)<<19);
    }
public:
    void invalidate() noexcept {initialized_=false;}
    void draw(GenesisPanel& lcd,const integrated::State& s) noexcept {
        std::array<bool,count> marks{};
        const auto mark=[&](int x,int y){
            if(x>=0&&x<width&&y>=0&&y<height)marks[std::size_t(y)*width+x]=true;
        };
        for(const auto& c:s.world.founders){
            if(c.x>=width||c.y>=height)continue;
            // The 16px founder sprite extends into adjacent 8px tiles.
            for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)
                mark(int(c.x)+dx,int(c.y)+dy);
        }
        for(std::size_t i=0;i<s.world.descendant_count;++i)
            mark(s.world.descendants[i].x,s.world.descendants[i].y);
        for(const auto& site:s.sites.sites)
            if(site.stage!=construction::Stage::empty&&site.tile<count)marks[site.tile]=true;
        lcd.startWrite();
        for(std::size_t i=0;i<count;++i){
            const auto next=signature(view(s,i))|(marks[i]?occupied:0u);
            if(!initialized_||next!=previous_[i]||marks[i]||(previous_[i]&occupied))
                draw_tile(lcd,s,i);
            previous_[i]=next;
        }
        for(unsigned k=0;k<construction::site_count;++k){
            const auto& site=s.sites.sites[k];
            if(site.stage==construction::Stage::empty||site.tile>=count)continue;
            const int x=(site.tile%width)*tile_px,y=(site.tile/width)*tile_px;
            const auto color=site_color(construction::kind_at(k));
            lcd.drawRect(x+1,y+1,6,6,color);
            if(site.stage==construction::Stage::complete)lcd.fillRect(x+2,y+2,4,4,color);
        }
        for(std::size_t i=0;i<s.world.descendant_count;++i)citizen(lcd,s.world.descendants[i],false);
        for(const auto& c:s.world.founders)citizen(lcd,c,true);
        lcd.endWrite();initialized_=true;
    }
};
}
