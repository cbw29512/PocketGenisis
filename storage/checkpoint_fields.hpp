#pragma once
#include "world.hpp"
// The current canonical codec predates the optional Citizen drive fields.
// Fail closed rather than silently discard motivations or accept a probe
// save that would change the simulation after reload. Remove only when a
// versioned codec and explicit migration/replay tests cover these fields.
#if defined(PG_FATIGUE_PROBE) || defined(PG_REPRODUCTIVE_DRIVE_PROBE)
#error "Disposable Citizen drives are not supported by the canonical checkpoint codec"
#endif
#include "world_codec_fields.hpp"
