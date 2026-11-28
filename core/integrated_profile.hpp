#pragma once
// A single explicit configuration for the current integrated candidate.
// Include before any core headers; never mix these layouts across TUs.
#ifdef PG_FATIGUE_PROBE
#error "Include integrated_profile.hpp before configuring Citizen"
#endif
#define PG_HOUSING_PROBE
#define PG_AGRICULTURE_PROBE
#define PG_IRRIGATION_PROBE
#define PG_MULTI_FIELD_PROBE
#define PG_CROP_LOSS_PROBE
#define PG_LIVESTOCK_PROBE
#define PG_SOIL_FERTILITY_PROBE
#define PG_MANURE_PROBE
#define PG_FATIGUE_PROBE
#define PG_REPRODUCTIVE_DRIVE_PROBE
#include "terrain_renewal.hpp"
#include "world_admission.hpp"
