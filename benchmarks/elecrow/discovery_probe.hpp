#pragma once
#include "../../core/discovery_work.hpp"
#include <Arduino.h>
namespace genesis::benchmark {
inline bool discovery_probe(World& w) {
    initialize(w,42);discovery::State s{};Production p{};
    Serial.println("DISCOVERY PROBE TEST ONLY: accelerated 60-tick opportunities; two founders; no permanent save");
    constexpr const char* kinds[]={"none","started","failed","discovered","learned","taught","abandoned"};
    unsigned available_ticks[2]={0,0},urgent[2]={0,0},weak[2]={0,0},actions[2][5]{};
#ifdef PG_FAMILY_PROBE
    constexpr unsigned ticks=43200;
    Serial.println("FAMILY PROBE: normal conception and gestation; accelerated CPU execution");
    unsigned conceptions=0,live_births=0,care_ticks=0;
    std::uint64_t prior_due=0;
#else
    constexpr unsigned ticks=3600;
#endif
    for(unsigned i=0;i<ticks;++i) {
        discovery::Event event{};
        if(!discovery::step_with_research(w,s,p,event,discovery::probe_cadence)) {Serial.println("ERROR discovery world tick");return false;}
        for(unsigned j=0;j<2;++j) {
            const auto& c=w.founders[j];
            available_ticks[j]+=discovery::available(c);
            urgent[j]+=(c.hunger>=12||c.thirst>=12);weak[j]+=c.health<50;
            ++actions[j][static_cast<unsigned>(c.action)];
        }
        if(event.kind!=discovery::EventKind::none)
            Serial.printf("RESEARCH tick=%llu founder=%llu event=%s technology=%s known=%u/%u\n",
                static_cast<unsigned long long>(w.tick),static_cast<unsigned long long>(event.founder),
                kinds[static_cast<unsigned>(event.kind)],discovery::rules[event.tech].name,
                discovery::discovered(s),static_cast<unsigned>(discovery::tech_count));
#ifdef PG_FAMILY_PROBE
        // Founder-only reproduction is the only possible source of births in one simulated year.
        if(w.pregnancy_count&&w.pregnancies[0].due_tick!=prior_due) {
            prior_due=w.pregnancies[0].due_tick;++conceptions;
            Serial.printf("FAMILY_CONCEPTION tick=%llu parent1=%llu parent2=%llu due=%llu\n",
                static_cast<unsigned long long>(w.tick),
                static_cast<unsigned long long>(w.pregnancies[0].parent1),
                static_cast<unsigned long long>(w.pregnancies[0].parent2),
                static_cast<unsigned long long>(prior_due));
        }
        for(std::size_t j=0;j<w.descendant_count;++j) {
            const auto& child=w.descendants[j];
            if(child.birth_tick==w.tick) {
                ++live_births;
                Serial.printf("FAMILY_BIRTH tick=%llu id=%llu parent1=%llu parent2=%llu health=%u\n",
                    static_cast<unsigned long long>(w.tick),static_cast<unsigned long long>(child.id),
                    static_cast<unsigned long long>(child.parent1),static_cast<unsigned long long>(child.parent2),
                    unsigned(child.health));
            }
        }
        for(const auto& c:w.founders) care_ticks+=c.action==Action::care;
        if((i+1)%3600==0)
#else
        if((i+1)%600==0)
#endif
            Serial.printf("CIV_PROGRESS tick=%u discovered=%u/%u population=%u\n",
            i+1,discovery::discovered(s),static_cast<unsigned>(discovery::tech_count),static_cast<unsigned>(w.descendant_count+2));
        delay(1);
    }
    Serial.printf("DISCOVERY_RESULT attempts=%llu successes=%llu failures=%llu abandoned=%llu pending=%u distinct=%u founder1_mask=%u founder2_mask=%u\n",
        static_cast<unsigned long long>(s.attempts),static_cast<unsigned long long>(s.successes),
        static_cast<unsigned long long>(s.failures),static_cast<unsigned long long>(s.abandoned),static_cast<unsigned>(s.job.active),
        discovery::discovered(s),unsigned(s.known[0]),unsigned(s.known[1]));
    Serial.printf("RESEARCH_AVAILABILITY founder1=%u founder2=%u\n",available_ticks[0],available_ticks[1]);
    for(unsigned j=0;j<2;++j) {
        const auto& c=w.founders[j];
        Serial.printf("FOUNDER_DIAGNOSTIC id=%u health=%u hunger=%u thirst=%u x=%u y=%u curiosity=%u urgent_ticks=%u weak_ticks=%u idle=%u move=%u eat=%u drink=%u care=%u\n",
            j+1,unsigned(c.health),unsigned(c.hunger),unsigned(c.thirst),unsigned(c.x),unsigned(c.y),
            unsigned(c.personality[0]),urgent[j],weak[j],actions[j][0],actions[j][1],actions[j][2],actions[j][3],actions[j][4]);
    }
#ifdef PG_FAMILY_PROBE
    Serial.printf("FAMILY_RESULT conceptions=%u live_births=%u care_actions=%u living=%u deaths=%u lost_gestations=%llu pregnancies=%u\n",
        conceptions,live_births,care_ticks,unsigned(w.descendant_count+2),unsigned(w.archive_count),
        static_cast<unsigned long long>(w.lost_gestations),unsigned(w.pregnancy_count));
    if(!live_births||!care_ticks) {Serial.println("ERROR family probe did not demonstrate birth and care");return false;}
#endif
    if(!s.attempts||!s.successes||!discovery::discovered(s)||!s.known[0]||!s.known[1]) {
        Serial.println("ERROR discovery probe did not demonstrate knowledge for both founders");return false;
    }
    return valid_population(w)&&s.attempts==s.successes+s.failures+s.abandoned+unsigned(s.job.active);
}
}
