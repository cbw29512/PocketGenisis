#pragma once
#include "../../core/discovery.hpp"
#include <Arduino.h>
namespace genesis::benchmark {
inline bool discovery_probe(World& w) {
    initialize(w,42);discovery::State s{};Production p{};
    Serial.println("DISCOVERY PROBE TEST ONLY: accelerated 60-tick opportunities; two founders; no permanent save");
    constexpr const char* kinds[]={"none","started","failed","discovered","learned","taught"};
    for(unsigned i=0;i<3600;++i) {
        if(!step_world(w,p)) {Serial.println("ERROR discovery world tick");return false;}
        const auto event=discovery::advance(w,s,discovery::probe_cadence);
        if(event.kind!=discovery::EventKind::none)
            Serial.printf("RESEARCH tick=%llu founder=%llu event=%s technology=%s known=%u/%u\n",
                static_cast<unsigned long long>(w.tick),static_cast<unsigned long long>(event.founder),
                kinds[static_cast<unsigned>(event.kind)],discovery::rules[event.tech].name,
                discovery::discovered(s),static_cast<unsigned>(discovery::tech_count));
        if((i+1)%600==0) Serial.printf("CIV_PROGRESS tick=%u discovered=%u/%u population=%u\n",
            i+1,discovery::discovered(s),static_cast<unsigned>(discovery::tech_count),static_cast<unsigned>(w.descendant_count+2));
        delay(1);
    }
    Serial.printf("DISCOVERY_RESULT attempts=%llu successes=%llu failures=%llu pending=%u distinct=%u founder1_mask=%u founder2_mask=%u\n",
        static_cast<unsigned long long>(s.attempts),static_cast<unsigned long long>(s.successes),
        static_cast<unsigned long long>(s.failures),static_cast<unsigned>(s.job.active),
        discovery::discovered(s),unsigned(s.known[0]),unsigned(s.known[1]));
    return valid_population(w)&&s.attempts==s.successes+s.failures+unsigned(s.job.active);
}
}
