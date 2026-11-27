#pragma once
#include "discovery.hpp"

namespace genesis::construction {
enum Kind : std::uint8_t { campfire,hut,store,
#ifdef PG_AGRICULTURE_PROBE
    field,
#endif
#ifdef PG_IRRIGATION_PROBE
    channel,
#endif
    kind_count };
enum class Stage : std::uint8_t { empty,building,complete,abandoned };
struct Blueprint {const char* name;discovery::Tech knowledge;unsigned wood,stone,work;};
inline constexpr std::array<Blueprint,kind_count> blueprints{{
    {"Campfire",discovery::fire,4,2,20},
    {"Shelter",discovery::shelter,8,4,40},
    {"Food store",discovery::food_storage,6,3,30}
#ifdef PG_AGRICULTURE_PROBE
    ,{"Prepared field",discovery::farming,2,1,20}
#endif
#ifdef PG_IRRIGATION_PROBE
    ,{"Irrigation channel",discovery::irrigation,4,3,30}
#endif
}};
struct Site {
    Stage stage{};
    std::uint16_t tile{};
    std::uint8_t owner{},work{};
    std::uint64_t started{},finished{},last_work{};
};
// Isolated capacity extension: existing profiles retain their original layout.
#ifdef PG_HOUSING_PROBE
inline constexpr unsigned extra_huts=8;
#else
inline constexpr unsigned extra_huts=0;
#endif
#ifdef PG_MULTI_FIELD_PROBE
inline constexpr unsigned extra_fields=3;
#else
inline constexpr unsigned extra_fields=0;
#endif
inline constexpr unsigned site_count=kind_count+extra_huts+extra_fields,housing_capacity=2;
inline constexpr Kind kind_at(unsigned slot) noexcept {
#ifdef PG_MULTI_FIELD_PROBE
    if(slot>=kind_count+extra_huts) return field;
#endif
    return slot<kind_count?static_cast<Kind>(slot):hut;
}
#ifdef PG_AGRICULTURE_PROBE
inline constexpr unsigned field_count=1+extra_fields;
inline constexpr unsigned field_slot(unsigned i) noexcept {
    return i==0?unsigned(field):kind_count+extra_huts+i-1;
}
#endif
struct State {
    std::array<Site,site_count> sites{};
};
inline unsigned completed_huts(const State& s) noexcept {
    unsigned n=0;for(unsigned k=0;k<site_count;++k)
        n+=kind_at(k)==hut&&s.sites[k].stage==Stage::complete;
    return n;
}
inline unsigned housing_demand(const World& w,const State& s) noexcept {
    const auto living=static_cast<unsigned>(w.descendant_count+2);
    const auto capacity=completed_huts(s)*housing_capacity;
    return living>capacity?living-capacity:0;
}
enum class EventKind : std::uint8_t { none,started,completed,abandoned };
struct Event {EventKind kind{};std::uint8_t type{};std::uint16_t tile{};std::uint64_t builder{};};
inline std::uint16_t location(const Citizen& c) noexcept {
    return static_cast<std::uint16_t>(std::size_t(c.y)*width+c.x);
}
inline int active(const State& s) noexcept {
    for(unsigned k=0;k<site_count;++k) if(s.sites[k].stage==Stage::building) return int(k);
    return -1;
}
inline bool occupied(const State& s,std::uint16_t tile) noexcept {
    for(const auto& b:s.sites) if(b.stage!=Stage::empty&&b.tile==tile) return true;
    return false;
}
inline bool near_completed_hut(const State& s,const Citizen& c) noexcept {
    for(unsigned j=0;j<site_count;++j) {
        const auto& home=s.sites[j];
        if(kind_at(j)==hut&&home.stage==Stage::complete&&
           std::abs(int(c.x)-int(home.tile%width))+std::abs(int(c.y)-int(home.tile/width))<=8) return true;
    }
    return false;
}
#ifdef PG_IRRIGATION_PROBE
inline constexpr unsigned irrigation_target=24,irrigation_source_reserve=24;
inline bool channel_adjacent(std::uint16_t a,std::uint16_t b) noexcept {
    return std::abs(int(a%width)-int(b%width))+std::abs(int(a/width)-int(b/width))==1;
}
// Stable lowest-water adjacent completed field; never select unrelated tiles.
inline int thirsty_field(const World& w,const State& s,std::uint16_t source) noexcept {
    int best=-1;unsigned lowest=irrigation_target;
    for(unsigned i=0;i<field_count;++i) {
        const auto k=field_slot(i);const auto& plot=s.sites[k];
        if(plot.stage!=Stage::complete||plot.tile>=w.tiles.size()||
           !channel_adjacent(source,plot.tile)) continue;
        if(w.tiles[plot.tile].water<lowest) {lowest=w.tiles[plot.tile].water;best=int(k);}
    }
    return best;
}
#endif
inline bool can_start(const World& w,const discovery::State& research,const State& s,
                      unsigned owner,unsigned k) noexcept {
    if(owner>=w.founders.size()||k>=site_count) return false;
    const auto& c=w.founders[owner];const auto& bp=blueprints[kind_at(k)];
    const auto tile=location(c);const auto& stock=w.tiles[tile];
    if(k>=kind_count&&kind_at(k)==hut&&(!housing_demand(w,s)||stock.food<12||stock.water<12||
                       !near_completed_hut(s,c))) return false;
#ifdef PG_AGRICULTURE_PROBE
    if(kind_at(k)==field&&(stock.food<14||stock.water<12||(!w.descendant_count&&stock.food>48)||
                 !near_completed_hut(s,c))) return false;
#ifdef PG_MULTI_FIELD_PROBE
    if(kind_at(k)==field&&k!=field) {
        unsigned established=0;
        for(unsigned i=0;i<field_count;++i)
            established+=s.sites[field_slot(i)].stage==Stage::complete;
        // Provisional planning demand, not a population cap or guaranteed yield.
        if(!established||w.descendant_count+2<=4*established||stock.food>48) return false;
    }
#endif
#endif
#ifdef PG_IRRIGATION_PROBE
    if(k==channel) {
        if(stock.water<=irrigation_source_reserve||thirsty_field(w,s,tile)<0) return false;
    }
#endif
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
            if(++site.work==blueprints[kind_at(unsigned(k))].work) {
                site.stage=Stage::complete;site.finished=w.tick;
                return {EventKind::completed,static_cast<std::uint8_t>(kind_at(unsigned(k))),site.tile,c.id};
            }
        } else if(w.tick-site.last_work>=600) {
            site.stage=Stage::abandoned;site.finished=w.tick;
            return {EventKind::abandoned,static_cast<std::uint8_t>(kind_at(unsigned(k))),site.tile,c.id};
        }
        return {};
    }
    // Stable small catalog and founder order; no population-wide scan.
    for(unsigned i=0;i<2;++i) for(unsigned j=0;j<site_count;++j) {
        if(!can_start(w,research,s,i,j)) continue;
        const auto tile=location(w.founders[i]);auto& stock=w.tiles[tile];const auto& bp=blueprints[kind_at(j)];
        stock.wood-=bp.wood;stock.stone-=bp.stone;
        s.sites[j]={Stage::building,tile,static_cast<std::uint8_t>(i),0,w.tick,0,w.tick};
        return {EventKind::started,static_cast<std::uint8_t>(kind_at(j)),tile,w.founders[i].id};
    }
    return {};
}
}
