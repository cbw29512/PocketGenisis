#pragma once
#ifdef PG_SHELTER_PROBE
#include "../../core/shelter.hpp"
#elif defined(PG_COOKING_PROBE)
#include "../../core/cooking.hpp"
#elif defined(PG_STORAGE_PROBE)
#include "../../core/food_store.hpp"
#else
#include "../../core/construction_work.hpp"
#endif
#include <Arduino.h>
namespace genesis::benchmark {
inline bool construction_probe(World& w) {
    initialize(w,42);discovery::State knowledge{};construction::State sites{};Production p{};
    unsigned births=0;
#ifdef PG_SHELTER_PROBE
    shelter::State sheltered{};
    Serial.println("FUNCTIONAL SHELTER PROBE");
#endif
#ifdef PG_COOKING_PROBE
    cooking::State cooked{};
    Serial.println("FUNCTIONAL CAMPFIRE PROBE");
#endif
#ifdef PG_STORAGE_PROBE
    storage::State reserve{};
    Serial.println("FUNCTIONAL FOOD STORAGE PROBE");
#endif
    Serial.println("CONSTRUCTION PROBE TEST ONLY: 43200 ticks; accelerated research; normal lifecycle; no save");
    constexpr const char* events[]={"none","started","completed","abandoned"};
    for(unsigned i=0;i<43200;++i) {
        discovery::Event re{};construction::Event ce{};
#ifdef PG_SHELTER_PROBE
        if(!shelter::step(w,knowledge,sites,reserve,cooked,sheltered,p,re,ce,discovery::probe_cadence)) {
#elif defined(PG_COOKING_PROBE)
        if(!cooking::step(w,knowledge,sites,reserve,cooked,p,re,ce,discovery::probe_cadence)) {
#elif defined(PG_STORAGE_PROBE)
        if(!storage::step(w,knowledge,sites,reserve,p,re,ce,discovery::probe_cadence)) {
#else
        if(!construction::step(w,knowledge,sites,p,re,ce,discovery::probe_cadence)) {
#endif
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
#ifdef PG_STORAGE_PROBE
    Serial.printf("STORAGE_RESULT balance=%u deposits=%llu withdrawals=%llu spoiled=%llu assisted=%llu state_bytes=%u\n",
        reserve.balance,static_cast<unsigned long long>(reserve.deposited),static_cast<unsigned long long>(reserve.withdrawn),
        static_cast<unsigned long long>(reserve.spoiled),static_cast<unsigned long long>(reserve.assisted),unsigned(sizeof(reserve)));
    if(!storage::valid(reserve)||!reserve.deposited) {Serial.println("ERROR food reserve demonstration");return false;}
#endif
#ifdef PG_COOKING_PROBE
    Serial.printf("COOKING_RESULT meals=%llu fuel_wood=%llu extra_hunger_relief=%llu state_bytes=%u\n",
        static_cast<unsigned long long>(cooked.meals),static_cast<unsigned long long>(cooked.wood),
        static_cast<unsigned long long>(cooked.relief),unsigned(sizeof(cooked)));
    if(!cooking::valid(cooked)) {Serial.println("ERROR cooking accounting");return false;}
#endif
#ifdef PG_SHELTER_PROBE
    Serial.printf("SHELTER_RESULT extra_recovery=%llu state_bytes=%u\n",
        static_cast<unsigned long long>(sheltered.extra_recovery),unsigned(sizeof(sheltered)));
#endif
    return valid_population(w)&&construction::completed(sites)>0&&births>0;
}
}
