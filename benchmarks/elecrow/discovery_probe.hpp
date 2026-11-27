#pragma once
#include "../../core/discovery_work.hpp"
#include <Arduino.h>
namespace genesis::benchmark {
inline bool discovery_probe(World& w) {
    initialize(w,42);discovery::State s{};Production p{};
    Serial.println("DISCOVERY PROBE TEST ONLY: accelerated 60-tick opportunities; two founders; no permanent save");
    constexpr const char* kinds[]={"none","started","failed","discovered","learned","taught","abandoned"};
    unsigned available_ticks[2]={0,0},urgent[2]={0,0},weak[2]={0,0},actions[2][5]{};
    for(unsigned i=0;i<3600;++i) {
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
        if((i+1)%600==0) Serial.printf("CIV_PROGRESS tick=%u discovered=%u/%u population=%u\n",
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
    if(!s.attempts||!s.successes||!discovery::discovered(s)||!s.known[0]||!s.known[1]) {
        Serial.println("ERROR discovery probe did not demonstrate knowledge for both founders");return false;
    }
    return valid_population(w)&&s.attempts==s.successes+s.failures+s.abandoned+unsigned(s.job.active);
}
}
