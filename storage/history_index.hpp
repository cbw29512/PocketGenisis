#pragma once
#include "host_history.hpp"
#include <map>

namespace genesis::storage {
// Host reference implementation only: O(total history) memory, not the ESP32 index.
class HistoryIndex {
    std::uint64_t seed_{}, count_{};
    std::map<std::uint64_t,DeathRecord> records_;
    const DeathRecord* last_{};
    HistoryIndex()=default;
public:
    HistoryIndex(const HistoryIndex&)=delete;
    HistoryIndex& operator=(const HistoryIndex&)=delete;
    // std::map move construction preserves references to its elements.
    HistoryIndex(HistoryIndex&&)=default;
    static HistoryIndex load(const std::string& dir,std::uint64_t seed,std::uint64_t count) {
        HistoryIndex index;index.seed_=seed;
        // Only the declared committed prefix is authoritative; ignore uncommitted suffix.
        while(index.count_<count) {
            const auto s=load_segment(dir,seed,index.count_);
            if(s.records.size()>count-index.count_) throw std::runtime_error("history crosses committed boundary");
            for(const auto& d:s.records) {
                const auto* p=index.last_;
                if(p&&(p->death_tick>d.death_tick||(p->death_tick==d.death_tick&&p->citizen.id>=d.citizen.id)))
                    throw std::runtime_error("history chronology conflict");
                const auto result=index.records_.emplace(d.citizen.id,d);
                if(!result.second) throw std::runtime_error("duplicate historical identity");
                index.last_=&result.first->second;
                ++index.count_;
            }
        }
        return index;
    }
    std::uint64_t seed() const noexcept {return seed_;}
    std::uint64_t count() const noexcept {return count_;}
    const auto& records() const noexcept {return records_;}
    template<class Visit> bool visit(const Visit& visitor) const {
        for(const auto& item:records_) if(!visitor(item.second)) return false;
        return true;
    }
    const DeathRecord* last() const noexcept {return last_;}
    const DeathRecord* find(std::uint64_t id) const noexcept {
        const auto it=records_.find(id);return it==records_.end()?nullptr:&it->second;
    }
};
// Shared combined validation: host map or checked disk-backed lookup.
template<class Index> inline bool valid_history_view(const World& w,const Index& index) {
    if(w.seed!=index.seed()||w.descendant_count>descendant_capacity||w.archive_count>history_capacity||
       w.next_citizen_id<3||index.count()>w.next_citizen_id-3||
       w.next_citizen_id-3-index.count()!=w.descendant_count+w.archive_count||
       !valid_founders(w)) return false;
    // All identities in range + no duplicates + exact count imply no missing IDs.
    auto mortal=[&](const Citizen& c) {
        return c.id>=3&&c.id<w.next_citizen_id&&valid_pair(c.parent1,c.parent2)&&c.parent2<c.id&&
               c.birth_tick&&c.birth_tick<=w.tick&&c.lifespan_years>=60&&c.lifespan_years<=80&&valid_agent(c);
    };
    if(!index.visit([&](const DeathRecord& d) {
        return mortal(d.citizen)&&d.death_tick<=w.tick;
    })) return false;
    std::uint64_t previous=2;
    for(std::size_t i=0;i<w.descendant_count;++i) {
        const auto& c=w.descendants[i];
        if(!mortal(c)||c.id<=previous||!c.health||c.birth_outcome==BirthOutcome::fatal||index.find(c.id)) return false;
        if(life_stage(c,w.tick)<LifeStage::adult&&c.purpose!=Purpose::none) return false;
        previous=c.id;
    }
    const auto* last=index.last();
    for(std::size_t i=0;i<w.archive_count;++i) {
        const auto& d=w.archive[i];const auto& c=d.citizen;
        if(!mortal(c)||d.death_tick<c.birth_tick||d.death_tick>w.tick||index.find(c.id)||live_citizen(w,c.id)||
           static_cast<unsigned>(d.cause)<1||static_cast<unsigned>(d.cause)>5||
           (d.cause==DeathCause::old_age?c.health==0:c.health!=0)||
           (d.cause==DeathCause::disease&&!is_ill(c))||
           ((d.cause==DeathCause::childbirth)!=(c.birth_outcome==BirthOutcome::fatal))||
           (d.cause==DeathCause::childbirth&&d.death_tick!=c.birth_tick)) return false;
        if(last&&(d.death_tick<last->death_tick||(d.death_tick==last->death_tick&&c.id<=last->citizen.id))) return false;
        for(std::size_t j=0;j<i;++j) if(w.archive[j].citizen.id==c.id) return false;
        last=&d;
    }
    auto lookup=[&](std::uint64_t id) -> PersonRecord {
        if(const auto* c=live_citizen(w,id)) return {c,false,0};
        if(const auto* d=index.find(id)) return {&d->citizen,true,d->death_tick};
        for(std::size_t i=0;i<w.archive_count;++i) {
            const auto& d=w.archive[i];if(d.citizen.id==id) return {&d.citizen,true,d.death_tick};
        }
        return {};
    };
    if(!index.visit([&](const DeathRecord& d) {
        return valid_genealogy(d.citizen,d.death_tick,w.next_citizen_id,lookup);
    })) return false;
    for(std::size_t i=0;i<w.archive_count;++i) {
        const auto& d=w.archive[i];
        if(!valid_genealogy(d.citizen,d.death_tick,w.next_citizen_id,lookup)) return false;
    }
    for(std::size_t i=0;i<w.descendant_count;++i)
        if(!valid_genealogy(w.descendants[i],w.tick,w.next_citizen_id,lookup)) return false;
    return valid_gestations(w);
}
} // namespace genesis::storage
