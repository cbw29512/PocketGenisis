#pragma once
#include "../../core/ecology.hpp"
#include <Arduino.h>
#include <esp_timer.h>
#include <algorithm>

namespace genesis::benchmark {
inline constexpr std::size_t phase_count=static_cast<std::size_t>(TickPhase::count);
inline constexpr const char* phase_names[]={"survival_validation","disease","mortality","lifecycle","renewal"};
static_assert(sizeof(phase_names)/sizeof(phase_names[0])==phase_count);
struct Profile {
    std::array<std::uint64_t,phase_count> sum{},peak{},current{},slowest{};
    std::uint64_t worst{},worst_tick{};
    bool step(World& w,Production& p,std::uint64_t& elapsed) noexcept {
        current.fill(0);
        const auto begin=esp_timer_get_time();
        auto last=begin;
        const auto ok=detail::step_world_observed(w,p,valid_population,[&](TickPhase phase) noexcept {
            const auto now=esp_timer_get_time();
            current[static_cast<std::size_t>(phase)]=static_cast<std::uint64_t>(now-last);
            last=now;
        });
        elapsed=static_cast<std::uint64_t>(esp_timer_get_time()-begin);
        if(!ok) return false;
        for(std::size_t i=0;i<phase_count;++i) {
            sum[i]+=current[i];peak[i]=std::max(peak[i],current[i]);
        }
        if(elapsed>worst) {worst=elapsed;worst_tick=w.tick;slowest=current;}
        return true;
    }
    void report(unsigned samples) const {
        for(std::size_t i=0;i<phase_count;++i)
            Serial.printf("PHASE name=%s mean_us=%llu max_us=%llu at_worst_tick_us=%llu\n",
                phase_names[i],static_cast<unsigned long long>(sum[i]/samples),
                static_cast<unsigned long long>(peak[i]),static_cast<unsigned long long>(slowest[i]));
        Serial.printf("WORST_TICK tick=%llu total_us=%llu\n",
            static_cast<unsigned long long>(worst_tick),static_cast<unsigned long long>(worst));
    }
};
}
