# Terrain traversal candidate — 2026-11-28

Existing observer firmware paints potential runoff habitat as blue water. It does not represent actual depth. Legacy renderer correction is isolated at integrated-recovery commit 9752df8; it preserves established topology. Never silently turn occupied legacy land into water or move citizens.

## Candidate state and invariants
- Explicit surface (land, shallow, deep), integer elevation, and cover. Resource water stock and runoff are not depth. New maps must provide measured/generated depth independently.
- Every walking edge is cardinal, in bounds, and has walkable endpoints; cliff rise greater than 32 quantized units blocks both directions. No diagonal, wrap, or water teleport.
- Provisional action costs: open 1, forest 2, highland or wading 3, gentle slope +2 uphill/+1 downhill, steep +3/+2. Maximum 6. These are game balancing constants pending calibration.
- Deterministic weighted shortest route; fixed caller-owned scratch (four arrays of 6000 uint16_t plus size), no live RNG. Do not run a full search for every citizen on every tick; qualify cache/replanning and mature population performance.
- Each citizen needs an explicit checkpointed TravelProgress; one attempt per simulation tick, pay all turns before changing coordinates, cancel if target/edge becomes invalid. No implicit migration of old PGCI checkpoints.
- Existing species catalog unchanged. No boat, bridge, swimming or flight inferred from species or labels.

## Verified targeted host evidence
Candidate commit dbef52a29ebbae5cdae1c5bcb126c57bf3243e2f. Direct clone failed DNS; reconstructed exact seven source files locally, verified Git blob SHA against remote contents:
- terrain_navigation.hpp e842d84fb47ceeea3e84b9075139d9f14fb4f1d2
- terrain_travel.hpp 5ff25916016b35036ee165310756f21fc913bda8
- terrain_navigation_test.cpp f617da8f434a486979e8f46c28bb8763df9589d3
- walkability.hpp 9df5260afb33a97ea7ebe44123bf20e7e32b4839
- hydrology.hpp a6584d23ce26fc67c2069ca3e8f26797d82c55fb
- world.hpp 9eb175038b685a2e218e72a7359599d36c769420
- rng.hpp 890743463d9022d6a161b0f2d997a3315e0456e7

GNU g++ 14.2.0, C++17, -Wall -Wextra -Werror -pedantic -fsanitize=undefined -fno-sanitize-recover=undefined. Ran tests/terrain_navigation_test.cpp twice: default and -DPG_FATIGUE_PROBE -DPG_REPRODUCTIVE_DRIVE_PROBE. Both exit 0: PASS terrain costs, path barriers, detours and real movement turns. These are targeted unit fixtures, not full integrated simulation/device tests.

## Remaining release work (existing geography and movement scope)
- [ ] Authoritative ocean/lake/river depth, terrain generation and immutable versioned topology; verify multi-seed connected land, genuine islands, fresh vs salt water, basin/flow and depth balance.
- [ ] Founding admission uses exact legal edge connectivity (including cliffs), local reachable resources, safe dry spawn; no guaranteed survival.
- [ ] Route-aware resource search and work targeting. Current survival uses nearest Manhattan ring and x-first move_toward, which cannot navigate barriers. Reconcile founder reunion, dependent following, construction/research/farm/livestock/manure work, wildlife migration and all direct coordinate mutations. Ensure care, births, and construction cannot cross barriers or occupy illegal surfaces.
- [ ] Save/version travel progress and topology; deterministic replay across reload; separate disposable test before any explicit migration. Never retroactively flood or relocate existing world.
- [ ] Actual discovered/constructed bridge and raft capability with materials, access, use and conservation. Ocean vessel requirements remain staged per approved progression.
- [ ] Terrain-dependent fatigue, health, carrying, weather/risk and actual ecology/resource rules; tune penalties, do not add unapproved mechanics or bird species.
- [ ] Shared renderer derives surface and elevation from authoritative navigation map; no blue false ocean. Add physical device and 32-seed multi-generation checks; validate path CPU/memory at larger populations.
