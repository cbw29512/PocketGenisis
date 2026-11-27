#pragma once
#include "../../core/construction_work.hpp"
#include <Arduino.h>
namespace genesis::benchmark {
inline bool construction_probe(World& w) {
    initialize(w,42);discovery::State knowledge{};construction::State sites{};Production p{};
    unsigned births=0;
    Serial.println("CONSTRUCTION PROBE TEST ONLY: 43200 ticks; accelerated research; normal lifecycle; no save");
    constexpr const char* events[]={"none","started","completed","abandoned"};
    for(unsigned i=0;i<43200;++i) {
        discovery::Event re{};construction::Event ce{};
        if(!construction::step(w,knowledge,sites,p,re,ce,discovery::probe_cadence)) {
            Serial.println("ERROR construction world tick");return false;
        }
        if(ce.kind!=construction::EventKind::none)
            Serial.printf("BUILD tick=%llu event=%s type=%s builder=%llu x=%u y=%u\n",
                static_cast<unsigned long long>(w.tick),events[static_cast<unsigned>(ce.kind)],
                construction::blueprints[ce.type].name,static_cast<unsigned long long>(ce.builder),
                unsigned(ce.tile%width),unsigned(ce.tile/width));
        for(std::size_t j=0;j<w.descendant_count;++j) if(w.descendants[j].birth_tick==w.tick) {
            ++births;Serial.printf("FAMILY_BIRTH tick=%llu id=%llu\n",
                static_cast<unsigned long long>(w.tick),static_cast<unsigned long long>(w.descendants[j].id));
        }
        if((i+1)%3600==0) Serial.printf("CIV_PROGRESS tick=%u discovered=%u buildings=%u living=%u\n",
            i+1,discovery::discovered(knowledge),construction::completed(sites),unsigned(w.descendant_count+2));
        delay(1);
    }
    Serial.printf("CONSTRUCTION_RESULT complete=%u discoveries=%u births=%u living=%u deaths=%u state_bytes=%u\n",
        construction::completed(sites),discovery::discovered(knowledge),births,unsigned(w.descendant_count+2),
        unsigned(w.archive_count),unsigned(sizeof(sites)));
    return valid_population(w)&&construction::completed(sites)>0&&births>0;
}
}
