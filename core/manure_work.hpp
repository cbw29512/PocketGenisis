#pragma once
#include "livestock.hpp"
namespace genesis::manure {
inline constexpr unsigned amendment_threshold=40;
inline bool qualified(const discovery::State& knowledge,const Citizen& c) noexcept {
    return is_founder(c)&&discovery::available(c)&&
           (knowledge.known[c.id-1]&discovery::tech_bit(discovery::soil_amendment));
}
inline int target_field(const World& w,const construction::State& sites,
                        const multi_field::State& farms,const Citizen& c) noexcept {
    int selected=-1,best=width+height;
    for(unsigned i=0;i<construction::field_count;++i) {
        const auto& site=sites.sites[construction::field_slot(i)];
        const auto& crop=farms.crops[i];
        if(site.stage!=construction::Stage::complete||site.tile>=w.tiles.size()||
           crop.stage||crop.nutrients.fertility>=amendment_threshold||
           w.tiles[site.tile].water<12) continue;
        const int d=std::abs(int(c.x)-int(site.tile%width))+std::abs(int(c.y)-int(site.tile/width));
        if(d<best) {best=d;selected=static_cast<int>(i);}
    }
    return selected;
}
// Selection is read-only except for ordinary movement. Reserve one
// local transaction for the authoritative post-survival labor phase.
enum class Task : std::uint8_t {none,collect,apply};
inline bool seek(const World& w,const discovery::State& knowledge,
                 const construction::State& sites,const multi_field::State& farms,
                 const Store& s,Citizen& c,Task& task) noexcept {
    if(!qualified(knowledge,c)) return false;
    const auto owner=unsigned(c.id-1);
    const auto field=target_field(w,sites,farms,c);
    if(field<0) return false;
    std::size_t destination{};
    Task intended{};
    if(s.carried[owner]) {
        destination=sites.sites[construction::field_slot(unsigned(field))].tile;
        intended=Task::apply;
    } else {
        if(!s.balance||!livestock::ready_pen(w,sites)) return false;
        destination=sites.sites[construction::enclosure].tile;intended=Task::collect;
    }
    if(construction::location(c)==destination) {task=intended;return true;}
    if(c.hunger>8||c.thirst>8) return false;
    move_toward(c,destination%width,destination/width);return true;
}
inline bool work(const World& w,const discovery::State& knowledge,
                 const construction::State& sites,multi_field::State& farms,
                 Store& s,const Citizen& c,Task task) noexcept {
    if(!qualified(knowledge,c)) return false;
    const auto owner=unsigned(c.id-1);
    const auto field=target_field(w,sites,farms,c);
    if(field<0) return false;
    if(task==Task::collect) {
        if(!livestock::ready_pen(w,sites)||
           construction::location(c)!=sites.sites[construction::enclosure].tile) return false;
        return collect(s,owner);
    }
    if(task!=Task::apply||!s.carried[owner]) return false;
    const auto i=unsigned(field);
    if(construction::location(c)!=sites.sites[construction::field_slot(i)].tile) return false;
    auto& nutrients=farms.crops[i].nutrients;
    // A low-fertility target always has room for a full nutrient unit.
    --s.carried[owner];++s.applied;
    nutrients.fertility+=nutrient_value;nutrients.amended+=nutrient_value;
    return true;
}
}
