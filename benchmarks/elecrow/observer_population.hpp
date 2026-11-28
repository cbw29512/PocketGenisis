#pragma once
#include "../../core/world.hpp"
#include <algorithm>
namespace genesis::observer {
// Read-only projection of the currently resident history. Does not invent
// milestones or change mortality. Future external archive must supply its
// full lifetime counters rather than treating the resident tail as totals.
inline constexpr unsigned death_cause_count=6;
inline constexpr std::array<const char*,death_cause_count> death_labels{{
    "Starvation","Dehydration","Old age","Fatal birth","Disease","Parent childbirth"
}};
inline const char* cause_label(DeathCause cause) noexcept {
    const auto value=unsigned(cause);
    return value>=1&&value<=death_cause_count?death_labels[value-1]:"Unknown";
}
struct PopulationStats {
    std::size_t founders{},descendants{},deaths{},live_births{},fatal_births{},unknown{};
    std::uint64_t lost_gestations{};
    std::size_t pregnancies{};
    std::array<unsigned,5> stages{};
    std::array<unsigned,death_cause_count> causes{};
};
inline PopulationStats population_stats(const World& w) noexcept {
    PopulationStats p{};
    p.founders=w.founders.size();
    p.descendants=std::min(w.descendant_count,w.descendants.size());
    p.deaths=std::min(w.archive_count,w.archive.size());
    p.pregnancies=w.pregnancy_count;p.lost_gestations=w.lost_gestations;
    for(std::size_t i=0;i<p.descendants;++i)
        ++p.stages[unsigned(life_stage(w.descendants[i],w.tick))];
    for(std::size_t i=0;i<p.deaths;++i){
        const auto cause=unsigned(w.archive[i].cause);
        if(cause>=1&&cause<=death_cause_count)++p.causes[cause-1];
        else ++p.unknown;
    }
    p.fatal_births=p.causes[unsigned(DeathCause::childbirth)-1];
    p.live_births=p.descendants+p.deaths-p.fatal_births;
    return p;
}
inline constexpr unsigned deaths_per_page=5;
inline std::size_t death_pages(std::size_t count) noexcept {
    return count?1+(count-1)/deaths_per_page:1;
}
// Newest first. Existing immutable records are the source of truth.
inline const DeathRecord* death_at(const World& w,std::size_t page,unsigned row) noexcept {
    const auto count=std::min(w.archive_count,w.archive.size());
    if(page>=death_pages(count)||row>=deaths_per_page)return nullptr;
    const auto offset=page*deaths_per_page+row;
    return offset<count?&w.archive[count-1-offset]:nullptr;
}
}
