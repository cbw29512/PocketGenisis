#pragma once
#include "discovery.hpp"

namespace genesis::construction {
enum Kind : std::uint8_t { campfire,hut,store,kind_count };
enum class Stage : std::uint8_t { empty,building,complete,abandoned };
struct Blueprint {const char* name;discovery::Tech knowledge;unsigned wood,stone,work;};
inline constexpr std::array<Blueprint,kind_count> blueprints{{
    {"Campfire",discovery::fire,4,2,20},
    {"Shelter",discovery::shelter,8,4,40},
    {"Food store",discovery::food_storage,6,3,30}
}};
struct Site {
    Stage stage{};
    std::uint16_t tile{};
    std::uint8_t owner{},work{};
    std::uint64_t started{},finished{},last_work{};
};
struct State {
    std::array<Site,kind_count> sites{};
};
enum class EventKind : std::uint8_t { none,started,completed,abandoned };
struct Event {EventKind kind{};std::uint8_t type{};std::uint16_t tile{};std::uint64_t builder{};};
inline std::uint16_t location(const Citizen& c) noexcept {
    return static_cast<std::uint16_t>(std::size_t(c.y)*width+c.x);
}
inline int active(const State& s) noexcept {
    for(unsigned k=0;k<kind_count;++k) if(s.sites[k].stage==Stage::building) return int(k);
    return -1;
}
inline bool occupied(const State& s,std::uint16_t tile) noexcept {
    for(const auto& b:s.sites) if(b.stage!=Stage::empty&&b.tile==tile) return true;
    return false;
}
inline bool can_start(const World& w,const discovery::State& research,const State& s,
                      unsigned owner,unsigned k) noexcept {
    const auto& c=w.founders[owner];const auto& bp=blueprints[k];
    const auto tile=location(c);const auto& stock=w.tiles[tile];
    return s.sites[k].stage==Stage::empty&&!occupied(s,tile)&&
        discovery::available(c)&&(research.known[owner]&discovery::tech_bit(bp.knowledge))&&
        stock.wood>=bp.wood&&stock.stone>=bp.stone;
}
inline unsigned completed(const State& s) noexcept {
    unsigned total=0;for(const auto& site:s.sites) total+=site.stage==Stage::complete;return total;
}
inline Event advance(World& w,const discovery::State& research,State& s) noexcept {
    if(!w.tick) return {};
    const auto k=active(s);
    if(k>=0) {
        auto& site=s.sites[k];const auto& c=w.founders[site.owner];
        if(discovery::available(c)&&location(c)==site.tile) {
            site.last_work=w.tick;
            if(++site.work==blueprints[k].work) {
                site.stage=Stage::complete;site.finished=w.tick;
                return {EventKind::completed,static_cast<std::uint8_t>(k),site.tile,c.id};
            }
        } else if(w.tick-site.last_work>=600) {
            site.stage=Stage::abandoned;site.finished=w.tick;
            return {EventKind::abandoned,static_cast<std::uint8_t>(k),site.tile,c.id};
        }
        return {};
    }
    // Stable small catalog and founder order; no population-wide scan.
    for(unsigned i=0;i<2;++i) for(unsigned j=0;j<kind_count;++j) {
        if(!can_start(w,research,s,i,j)) continue;
        const auto tile=location(w.founders[i]);auto& stock=w.tiles[tile];const auto& bp=blueprints[j];
        stock.wood-=bp.wood;stock.stone-=bp.stone;
        s.sites[j]={Stage::building,tile,static_cast<std::uint8_t>(i),0,w.tick,0,w.tick};
        return {EventKind::started,static_cast<std::uint8_t>(j),tile,w.founders[i].id};
    }
    return {};
}
}
