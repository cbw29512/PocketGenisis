#pragma once
#ifdef PG_LIVESTOCK_PROBE
#include "../../core/livestock_integration.hpp"
#elif defined(PG_CROP_LOSS_PROBE)
#include "../../core/crop_loss.hpp"
#elif defined(PG_MULTI_FIELD_PROBE)
#include "../../core/multi_field.hpp"
#elif defined(PG_IRRIGATION_PROBE)
#include "../../core/irrigation.hpp"
#elif defined(PG_AGRICULTURE_PROBE)
#include "../../core/agriculture.hpp"
#elif defined(PG_SETTLEMENT_PROBE)
#include "../../core/settlement.hpp"
#elif defined(PG_SHELTER_PROBE)
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
inline bool construction_probe(World& w
#ifdef PG_LIVESTOCK_PROBE
    ,wildlife::Wildlife& wild
#endif
) {
    initialize(w,42);discovery::State knowledge{};construction::State sites{};Production p{};
    unsigned births=0;
#ifdef PG_LIVESTOCK_PROBE
    wildlife::initialize(wild,w.seed);wildlife::Totals ecology{};livestock::State managed{};
    const auto initial_wild=wildlife::population(wild);
    Serial.println("LIVESTOCK PROBE");
#endif
#ifdef PG_IRRIGATION_PROBE
    irrigation::State irrigation_water{};
    Serial.println("IRRIGATION PROBE");
#endif
#ifdef PG_AGRICULTURE_PROBE
#ifdef PG_MULTI_FIELD_PROBE
    multi_field::State farms{};
    auto& crops=farms.crops[0]; // existing serial diagnostics describe first field
#else
    agriculture::State crops{};
#endif
    Serial.println("AGRICULTURE PROBE");
#endif
#ifdef PG_SETTLEMENT_PROBE
    settlement::State home{};
    Serial.println("SETTLEMENT RETURN PROBE");
#endif
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
    constexpr const char* events[]={"none","started","completed","abandoned","resumed"};
    for(unsigned i=0;i<43200;++i) {
        discovery::Event re{};construction::Event ce{};
#ifdef PG_LIVESTOCK_PROBE
        if(!livestock::step(w,knowledge,sites,reserve,cooked,sheltered,home,farms,irrigation_water,wild,ecology,managed,p,re,ce,discovery::probe_cadence)) {
#elif defined(PG_CROP_LOSS_PROBE)
        if(!crop_loss::step(w,knowledge,sites,reserve,cooked,sheltered,home,farms,irrigation_water,p,re,ce,discovery::probe_cadence)) {
#elif defined(PG_MULTI_FIELD_PROBE)
        if(!multi_field::step(w,knowledge,sites,reserve,cooked,sheltered,home,farms,irrigation_water,p,re,ce,discovery::probe_cadence)) {
#elif defined(PG_IRRIGATION_PROBE)
        if(!irrigation::step(w,knowledge,sites,reserve,cooked,sheltered,home,crops,irrigation_water,p,re,ce,discovery::probe_cadence)) {
#elif defined(PG_AGRICULTURE_PROBE)
        if(!agriculture::step(w,knowledge,sites,reserve,cooked,sheltered,home,crops,p,re,ce,discovery::probe_cadence)) {
#elif defined(PG_SETTLEMENT_PROBE)
        if(!settlement::step(w,knowledge,sites,reserve,cooked,sheltered,home,p,re,ce,discovery::probe_cadence)) {
#elif defined(PG_SHELTER_PROBE)
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
#ifdef PG_SETTLEMENT_PROBE
    Serial.printf("SETTLEMENT_RESULT homeward_steps=%llu arrivals=%llu state_bytes=%u\n",
        static_cast<unsigned long long>(home.steps),static_cast<unsigned long long>(home.arrivals),unsigned(sizeof(home)));
    if(!settlement::valid(home)) {Serial.println("ERROR settlement accounting");return false;}
#endif
#ifdef PG_HOUSING_PROBE
    Serial.printf("HOUSING_RESULT shelters=%u planning_capacity=%u unmet_demand=%u site_slots=%u\n",
        construction::completed_huts(sites),construction::completed_huts(sites)*construction::housing_capacity,
        construction::housing_demand(w,sites),construction::site_count);
#endif
#ifdef PG_AGRICULTURE_PROBE
    Serial.printf("AGRICULTURE_RESULT known=%u fields=%u planted=%llu tended=%llu water=%llu harvests=%llu food=%llu stage=%u state_bytes=%u\n",
        discovery::discovered(knowledge),unsigned(sites.sites[construction::field].stage==construction::Stage::complete),
        static_cast<unsigned long long>(crops.plants),static_cast<unsigned long long>(crops.tended),
        static_cast<unsigned long long>(crops.water),static_cast<unsigned long long>(crops.harvests),
        static_cast<unsigned long long>(crops.food),unsigned(crops.stage),unsigned(sizeof(crops)));
    const auto plot=sites.sites[construction::field].tile;
    Serial.printf("FARM_DIAGNOSTIC plot=%u food=%u water=%u founder1_distance=%d founder2_distance=%d\n",
        unsigned(plot),unsigned(w.tiles[plot].food),unsigned(w.tiles[plot].water),
        std::abs(int(w.founders[0].x)-int(plot%width))+std::abs(int(w.founders[0].y)-int(plot/width)),
        std::abs(int(w.founders[1].x)-int(plot%width))+std::abs(int(w.founders[1].y)-int(plot/width)));
    if(!agriculture::valid(crops)) {Serial.println("ERROR agriculture accounting");return false;}
#endif
#ifdef PG_IRRIGATION_PROBE
    Serial.printf("IRRIGATION_RESULT channels=%u transfers=%llu water_moved=%llu state_bytes=%u\n",
        unsigned(sites.sites[construction::channel].stage==construction::Stage::complete),
        static_cast<unsigned long long>(irrigation_water.transfers),
        static_cast<unsigned long long>(irrigation_water.water),unsigned(sizeof(irrigation_water)));
    if(!irrigation::valid(irrigation_water)) {Serial.println("ERROR irrigation accounting");return false;}
#endif
#ifdef PG_MULTI_FIELD_PROBE
    Serial.printf("MULTI_FIELD_RESULT fields=%u harvests=%llu state_bytes=%u\n",
        multi_field::completed(sites),static_cast<unsigned long long>(multi_field::harvests(farms)),unsigned(sizeof(farms)));
    if(!multi_field::valid(farms)) {Serial.println("ERROR multi field accounting");return false;}
#endif
#ifdef PG_CROP_LOSS_PROBE
    std::uint64_t drought=0,spoilage=0,lost_water=0;
    for(const auto& crop:farms.crops) {
        drought+=crop.drought_losses;spoilage+=crop.spoilage_losses;lost_water+=crop.lost_water;
    }
    Serial.printf("CROP_LOSS_RESULT drought=%llu spoilage=%llu lost_water=%llu\n",
        static_cast<unsigned long long>(drought),static_cast<unsigned long long>(spoilage),
        static_cast<unsigned long long>(lost_water));
#endif
#ifdef PG_LIVESTOCK_PROBE
    Serial.printf("LIVESTOCK_RESULT pen=%u captures=%llu animals=%u meat=%llu wild=%llu state_bytes=%u\n",
        unsigned(sites.sites[construction::enclosure].stage==construction::Stage::complete),
        static_cast<unsigned long long>(livestock::captures(managed)),livestock::population(managed),
        static_cast<unsigned long long>(managed.meat_produced),static_cast<unsigned long long>(wildlife::population(wild)),unsigned(sizeof(managed)));
    if(!livestock::valid(managed)||!wildlife::valid(wild)||
       initial_wild+ecology.births!=wildlife::population(wild)+ecology.deaths+livestock::captures(managed)) {
        Serial.println("ERROR livestock accounting");return false;
    }
#endif
    return valid_population(w)&&construction::completed(sites)>0&&births>0;
}
}
