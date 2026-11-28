#pragma once
#ifndef PG_LIVESTOCK_PROBE
#error "Livestock requires its isolated probe profile"
#endif
#include "crop_loss.hpp"
#include "wildlife.hpp"
#include "livestock_fodder.hpp"

namespace genesis::livestock {
inline constexpr unsigned capacity=8,breeding_floor=3;
inline constexpr std::array<wildlife::Species,2> species{{wildlife::cattle,wildlife::chicken}};
struct Group {std::uint16_t count{};std::uint64_t captured{},births{},deaths{},harvested{};};
struct State {
    std::array<Group,species.size()> groups{};
    wildlife::Totals resources{}; // reuse grazing counters; no wild population changes here
    std::uint64_t meat_produced{},last_pulse{};
    std::uint64_t supplied_food{},supplied_water{};
    Fodder fodder{};
};
inline bool valid(const State& s) noexcept {
    std::uint64_t meat=0;
    for(unsigned i=0;i<species.size();++i) {
        const auto& g=s.groups[i];
        if(g.count>capacity||g.captured+g.births!=g.count+g.deaths+g.harvested) return false;
        meat+=g.harvested*wildlife::yield[species[i]];
    }
    return meat==s.meat_produced&&s.resources.food==s.resources.water&&valid(s.fodder);
}
inline unsigned population(const State& s) noexcept {
    unsigned n=0;for(const auto& g:s.groups)n+=g.count;return n;
}
inline std::uint64_t captures(const State& s) noexcept {
    std::uint64_t n=0;for(const auto& g:s.groups)n+=g.captured;return n;
}
inline bool ready_pen(const World& w,const construction::State& sites) noexcept {
    const auto& pen=sites.sites[construction::enclosure];
    return pen.stage==construction::Stage::complete&&pen.tile<w.tiles.size();
}
inline void mark_development(const World& w,const construction::State& sites,wildlife::Wildlife& wild) noexcept {
    // No crowd-based development; only actual completed physical structures.
    for(const auto& site:sites.sites)
        if(site.stage==construction::Stage::complete&&site.tile<w.tiles.size())
            wild.developed[site.tile]=1;
}
inline constexpr unsigned supply_target=80,supply_batch=4,supply_reserve=24;
enum class Task : std::uint8_t {none,capture,harvest,supply_food,supply_water,store_fodder};
struct Choice {Task task{};unsigned group{};};
inline Choice choose(const World& w,const discovery::State& knowledge,const construction::State& sites,
                     const wildlife::Wildlife& wild,const State& s,const Citizen& c,bool winter_reserve=false) noexcept {
    if(!ready_pen(w,sites)||!is_founder(c)||!discovery::available(c)||
       !(knowledge.known[c.id-1]&discovery::tech_bit(discovery::husbandry))) return {};
    const auto pen=sites.sites[construction::enclosure].tile;
    const auto tile=construction::location(c);
    if(tile>=w.tiles.size()) return {};
    // Capture is a local transfer from an adjacent wild tile; no magic spawning.
    if(!wild.developed[tile]&&construction::channel_adjacent(tile,pen))
        for(unsigned i=0;i<species.size();++i)
            if(s.groups[i].count<2&&wild.tiles[tile].count[species[i]]) return {Task::capture,i};
    if(tile==pen&&w.tiles[pen].food<=48)
        for(unsigned i=0;i<species.size();++i)
            if(s.groups[i].count>breeding_floor&&wild.tiles[pen].meat+wildlife::yield[species[i]]<=wildlife::meat_limit)
                return {Task::harvest,i};
    // A handler at an undeveloped adjacent source can deliver a small
    // amount into the pen; never withdraw from an existing field or reserve.
    if(population(s)&&!wild.developed[tile]&&construction::channel_adjacent(tile,pen)) {
        const auto& from=w.tiles[tile];const auto& to=w.tiles[pen];
        if(to.food<supply_target&&from.food>supply_reserve) return {Task::supply_food,0};
        if(to.water<supply_target&&from.water>supply_reserve) return {Task::supply_water,0};
    }
    // Stockpile before winter, including a small initial reserve at a
    // completed but empty pen. Winter deliveries require living animals.
    if(winter_reserve&&(population(s)||season(w)==1||season(w)==2)&&
       !wild.developed[tile]&&construction::channel_adjacent(tile,pen)&&
       s.fodder.balance<fodder_target(population(s))&&w.tiles[tile].food>supply_reserve)
        return {Task::store_fodder,0};
    return {};
}
inline Task work(World& w,const discovery::State& knowledge,const construction::State& sites,
                 wildlife::Wildlife& wild,State& s,Citizen& c,bool winter_reserve=false) noexcept {
    const auto choice=choose(w,knowledge,sites,wild,s,c,winter_reserve);
    if(choice.task==Task::none) return Task::none;
    if(choice.task==Task::store_fodder) {
        if(!deposit_fodder(s.fodder,w.tiles[construction::location(c)],population(s),
                           supply_reserve,supply_batch)) return Task::none;
        return choice.task;
    }
    if(choice.task==Task::supply_food||choice.task==Task::supply_water) {
        auto& from=w.tiles[construction::location(c)];
        auto& to=w.tiles[sites.sites[construction::enclosure].tile];
        const bool food=choice.task==Task::supply_food;
        auto& source=food?from.food:from.water;auto& dest=food?to.food:to.water;
        const auto moved=resource_transfer::move(source,dest,supply_reserve,supply_target,supply_batch);
        if(!moved) return Task::none;
        (food?s.supplied_food:s.supplied_water)+=moved;
        return choice.task;
    }
    auto& g=s.groups[choice.group];const auto k=species[choice.group];
    if(choice.task==Task::capture) {
        --wild.tiles[construction::location(c)].count[k];++g.count;++g.captured;
    } else {
        --g.count;++g.harvested;
        wild.tiles[sites.sites[construction::enclosure].tile].meat+=wildlife::yield[k];
        s.meat_produced+=wildlife::yield[k];
    }
    return choice.task;
}
inline void pulse(World& w,const construction::State& sites,State& s,
                  wildlife::Cadence cadence=wildlife::original_cadence,bool winter_reserve=false) noexcept {
    if(!wildlife::valid_cadence(cadence)||!w.tick||w.tick%cadence.feeding||
       w.tick<=s.last_pulse||!ready_pen(w,sites)) return;
    auto& stock=w.tiles[sites.sites[construction::enclosure].tile];
    s.last_pulse=w.tick;
    const auto feed=[&](unsigned n) noexcept {
        return winter_reserve?feed_with_fodder(stock,n,s.fodder,s.resources):
                              wildlife::graze(stock,n,s.resources);
    };
    for(auto& g:s.groups) {
        // Existing wildlife graze debits food and water, no new resource system.
        const auto surviving=feed(g.count);
        g.deaths+=g.count-surviving;g.count=static_cast<std::uint16_t>(surviving);
        if(w.tick%cadence.breeding==0) {
            // Do not spend winter reserves on extra births while ordinary
            // food is scarce; preserve the breeding population first.
            const auto wanted=wildlife::offspring_request(surviving,capacity);
            const auto born=wildlife::graze(stock,wanted,s.resources);
            g.count+=born;g.births+=born;
        }
    }
}
}
