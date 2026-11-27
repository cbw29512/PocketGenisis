#pragma once
#include "world.hpp"
#include "crowding.hpp"
#include <algorithm>
#include <cstdlib>

namespace genesis {
// Fictional, simulation-only catalog. Durations are 60-tick pulses.
struct DiseaseRule { unsigned duration, immunity, damage; };
inline constexpr std::array<DiseaseRule,disease_count> disease_rules{{{6,720,8},{10,2160,12}}};
inline bool valid_diseases(const Citizen& c) noexcept {
    for(std::size_t k=0;k<disease_count;++k) {
        const auto& d=c.diseases[k];const auto& r=disease_rules[k];
        if(d.resistance>100||d.illness>r.duration||d.immunity>r.immunity||
           (d.illness&&d.immunity)) return false;
    }
    return true;
}
inline bool is_ill(const Citizen& c) noexcept {
    for(const auto& d:c.diseases) if(d.illness) return true;
    return false;
}
inline void inherit_resistance(World& w,Citizen& child,const Citizen& a,const Citizen& b) noexcept {
    for(std::size_t k=0;k<disease_count;++k) {
        std::uint32_t roll{};w.streams[immunity_traits].bounded(21,roll);
        const int mean=(a.diseases[k].resistance+b.diseases[k].resistance)/2;
        child.diseases[k].resistance=static_cast<std::uint8_t>(std::clamp(mean+int(roll)-10,0,100));
    }
}
inline unsigned exposure_risk(unsigned contacts,unsigned resistance) noexcept {
    return std::min(6000u,2000u*contacts)*(200u-resistance)/200u;
}
inline Citizen& disease_citizen(World& w,std::size_t i) noexcept {
    return i<2?w.founders[i]:w.descendants[i-2];
}
inline bool disease_roll(World& w,unsigned chance) noexcept {
    std::uint32_t roll{};w.streams[disease_events].bounded(10000,roll);
    return roll<chance;
}
inline void advance_diseases(World& w) noexcept {
    if(w.tick%60) return;
    const auto n=w.descendant_count+2;
    // Snapshot contagious sources before progression; prevent same-pulse cascades.
    std::array<std::array<bool,descendant_capacity+2>,disease_count> infectious{};
    std::array<bool,descendant_capacity+2> alive{};
    for(std::size_t i=0;i<n;++i) alive[i]=disease_citizen(w,i).health!=0;
    for(std::size_t k=0;k<disease_count;++k) {
        for(std::size_t i=0;i<n;++i) infectious[k][i]=alive[i]&&disease_citizen(w,i).diseases[k].illness!=0;
    }
    for(std::size_t k=0;k<disease_count;++k) {
        const auto& rule=disease_rules[k];
        const bool has_source=std::any_of(infectious[k].begin(),infectious[k].end(),[](bool x){return x;});
        bool any=false;
        for(std::size_t i=0;i<n;++i) {
            auto& c=disease_citizen(w,i);auto& d=c.diseases[k];
            if(!c.health) continue;
            if(d.illness) {
                const int floor=i<2?1:0;
                c.health=static_cast<std::uint8_t>(std::max(floor,int(c.health)-int(stressed_disease_damage(rule.damage,c.hunger,c.thirst))));
                // Preserve infection provenance at a fatal pulse.
                if(!c.health) continue;
                if(--d.illness==0&&disease_roll(w,7500)) d.immunity=static_cast<std::uint16_t>(rule.immunity);
            } else if(d.immunity) {
                --d.immunity;
            } else if(has_source) {
                unsigned contacts=0,neighbors=0;
                for(std::size_t j=0;j<n;++j) {
                    if(j==i||!alive[j]) continue;
                    const auto& source=disease_citizen(w,j);
                    if(std::abs(int(c.x)-int(source.x))+std::abs(int(c.y)-int(source.y))<=1) {
                        ++neighbors;
                        if(infectious[k][j]) ++contacts;
                    }
                }
                if(contacts&&disease_roll(w,crowded_exposure(exposure_risk(contacts,d.resistance),neighbors)))
                    d.illness=static_cast<std::uint16_t>(rule.duration);
            }
            any=any||d.illness!=0;
        }
        // Independent low-frequency environmental introduction, not a forced outbreak.
        if(!any&&w.tick%3600==0&&disease_roll(w,5)) {
            std::uint32_t selected{};w.streams[disease_events].bounded(static_cast<std::uint32_t>(n),selected);
            auto& c=disease_citizen(w,selected);auto& d=c.diseases[k];
            if(c.health&&!d.immunity&&!d.illness&&disease_roll(w,exposure_risk(1,d.resistance)))
                d.illness=static_cast<std::uint16_t>(rule.duration);
        }
    }
}
} // namespace genesis
