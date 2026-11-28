#pragma once
#include "history_validation.hpp"
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace genesis::storage {
// Short-lived cache for one closed replay transaction. It is not persisted or
// exposed as a substitute for full checkpoint validation.
class ReplayLineage {
    std::uint64_t seed_{},count_{},first_new_id_{};
    DeathRecord last_{};
    std::vector<DeathRecord> references_;
    std::array<std::uint64_t,history_capacity> resident_ids_{};
    std::size_t resident_count_{};
    ReplayLineage()=default;
public:
    template<class Index> static ReplayLineage build(const World& w,const Index& history) {
        if(!valid_history_view(w,history)) throw std::runtime_error("invalid replay origin");
        ReplayLineage cache;
        cache.seed_=w.seed;cache.count_=history.count();cache.first_new_id_=w.next_citizen_id;
        if(const auto* last=history.last()) cache.last_=*last;
        auto remember=[&](const Citizen& c) {
            cache.resident_ids_[cache.resident_count_++]=c.id;
            for(auto id:{c.parent1,c.parent2,c.guardian_id}) {
                if(id<3||cache.find(id)) continue;
                if(const auto* d=history.find(id)) {
                    if(cache.references_.size()>=3*history_capacity)
                        throw std::runtime_error("replay lineage cache capacity");
                    const auto pos=std::lower_bound(cache.references_.begin(),cache.references_.end(),id,
                        [](const DeathRecord& row,std::uint64_t key){return row.citizen.id<key;});
                    cache.references_.insert(pos,*d);
                }
            }
        };
        for(std::size_t i=0;i<w.descendant_count;++i) remember(w.descendants[i]);
        for(std::size_t i=0;i<w.archive_count;++i) remember(w.archive[i].citizen);
        std::sort(cache.resident_ids_.begin(),cache.resident_ids_.begin()+cache.resident_count_);
        return cache;
    }
    std::uint64_t seed() const noexcept {return seed_;}
    std::uint64_t count() const noexcept {return count_;}
    const DeathRecord* last() const noexcept {return count_?&last_:nullptr;}
    std::size_t reference_count() const noexcept {return references_.size();}
    const DeathRecord* find(std::uint64_t id) const noexcept {
        const auto pos=std::lower_bound(references_.begin(),references_.end(),id,
            [](const DeathRecord& row,std::uint64_t key){return row.citizen.id<key;});
        return pos!=references_.end()&&pos->citizen.id==id?&*pos:nullptr;
    }
    bool valid(const World& w) const noexcept {
        if(w.next_citizen_id<first_new_id_||w.descendant_count>descendant_capacity||
           w.archive_count>history_capacity) return false;
        // All pre-existing resident IDs were proven disjoint from external
        // history. New births have monotonic IDs; never accept another old ID.
        const auto allowed=[&](std::uint64_t id) {
            return id>=first_new_id_||std::binary_search(resident_ids_.begin(),
                resident_ids_.begin()+resident_count_,id);
        };
        for(std::size_t i=0;i<w.descendant_count;++i) if(!allowed(w.descendants[i].id)) return false;
        for(std::size_t i=0;i<w.archive_count;++i) if(!allowed(w.archive[i].citizen.id)) return false;
        return valid_resident_history(w,*this);
    }
};
} // namespace genesis::storage
