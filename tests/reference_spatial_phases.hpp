#pragma once
#include "ecology.hpp"
// Frozen pre-optimization phases from 6e15595 for deterministic parity only.
namespace genesis {
inline void reference_disease(World& w) noexcept {
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

inline void reference_conception(World& w) noexcept {
    if(w.tick%600||w.pregnancy_count>=std::numeric_limits<std::uint64_t>::max()-w.next_citizen_id||
       w.pregnancy_count>=pregnancy_capacity||
       w.descendant_count+w.pregnancy_count>=descendant_capacity||
       w.descendant_count+w.archive_count+w.pregnancy_count>=history_capacity||
       w.tick>std::numeric_limits<std::uint64_t>::max()-gestation_ticks) return;
    // Parents cannot participate in two simultaneous gestations.
    std::array<bool,live_capacity> busy{};
    for(std::size_t i=0;i<w.pregnancy_count;++i) {
        const auto& p=w.pregnancies[i];
        const auto a=live_slot(w,p.parent1),b=live_slot(w,p.parent2);
        if(a==live_capacity||b==live_capacity) return;
        busy[a]=busy[b]=true;
    }
    // Reservoir sampling: every locally eligible pair has equal selection chance.
    std::uint32_t count=0,roll{};
    Pregnancy selected{};
    const auto n=w.descendant_count+2;
    for(std::size_t i=0;i<n;++i) {
        const auto& a=active_at(w,i);
        if(busy[i]||!eligible_parent(a,w.tick)) continue;
        for(std::size_t j=i+1;j<n;++j) {
            const auto& b=active_at(w,j);
            if(busy[j]||!valid_pair(a.id,b.id)||!eligible_parent(b,w.tick)||
               std::abs(int(a.x)-int(b.x))+std::abs(int(a.y)-int(b.y))>2) continue;
            ++count;
            if(count==1||(w.streams[reproduction].bounded(count,roll)&&roll==0))
                selected={a.id,b.id,w.tick+gestation_ticks};
        }
    }
    if(count&&w.streams[reproduction].bounded(4,roll)&&roll==0)
        w.pregnancies[w.pregnancy_count++]=selected;
}

}
