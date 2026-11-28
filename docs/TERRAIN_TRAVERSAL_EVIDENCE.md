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

## Water-depth and founding admission increment
Candidate 2f39fe81674db00e314380d864290fa3b7bc007f. Added terrain_water.hpp: creation-only priority flood from perimeter, ocean connectivity and inland depression depth capped by spill saddle and provisional freshwater ceiling. Water stock and potential runoff are not depth. Added navigation_admission.hpp: flood-fill the same legal edges as weighted navigation (including cliff blocks), reject wet or disconnected founder spawns and count only reachable dry resource stocks. No change to legacy world geometry. Sea level 105, freshwater ceiling 125, wade depth 2 are provisional quantized elevation parameters, not physical metres. River channel depth/flow, conservation, rainfall and integration into actual procedural generation remain open.

Targeted GNU g++ 14.2.0 C++17 -Wall -Wextra -Werror -pedantic -fsanitize=undefined -fno-sanitize-recover=undefined build and execution passed in both default and -DPG_FATIGUE_PROBE -DPG_REPRODUCTIVE_DRIVE_PROBE layouts. Exact local source blob hashes matched all nine remote source/dependency files; new blobs terrain_water.hpp c61622a9c7ed5f0717a338cfe8ed0a680a9bd0c1, navigation_admission.hpp 723cd412f5635d81f2c29303928fbf73bc7ab194, terrain_water_test.cpp c09239c3578c3dc2a47093b042523155bef5d6c0. PASS ocean connectivity, freshwater depth and terrain-aware founder admission. Fixtures cover sea shallows, inland spill, sea connection, invalid policy no mutation, cliff-isolated stocks and saltwater excluded from dry founding resources. Not yet wired into generation, consumption, wildlife or the integrated tick. No full CI or device certification. Existing release items remain open.

## Reachable resource routing increment
Candidate 0c4f8d8552d37421c351e68d6f5315a58fcb4bd1. Generalized the existing deterministic weighted router to search for a goal predicate within a travel-turn budget. Added navigation_resource.hpp: search only accessible dry food and dry/shallow freshwater stocks; saltwater cannot qualify, submerged land food cannot qualify, no stock mutation during route selection. Deep freshwater bank access, fishing, hunting, reserves, and actual consumption must be reconciled with the existing integrated survival transaction. No change to current firmware or checkpoint rules.

Local GNU g++ 14.2.0 strict C++17 (-Wall -Wextra -Werror -pedantic -fsanitize=undefined -fno-sanitize-recover=undefined): terrain_navigation_test, terrain_water_test, navigation_resource_test each passed under default and fatigue/reproductive-drive layouts, six executed builds total. Initial resource fixture failed because a previously submerged food stock was left populated after converting its tile to dry land; corrected fixture, not algorithm. Verified new remote blobs match locally compiled sources: terrain_navigation.hpp acabbdd50c782e4e296e4046a27227040d7eafc2, navigation_resource.hpp 4643d999eaa7c387fa40b61249111db1bf380d99, navigation_resource_test.cpp 407d94804d6f35c90637cfcd7c8f90538b5aa9fb. Direct clone unavailable (DNS). Full integrated CI and hardware tests not executed. Next: reconcile navigation branch with integrated recovery branch; connect actual generated topology to the same authoritative state, all movement, food/consumption and save transactions. No release gate closed.
