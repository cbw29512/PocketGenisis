#pragma once
#include "host_history.hpp"
#include "history_validation.hpp"
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
} // namespace genesis::storage
