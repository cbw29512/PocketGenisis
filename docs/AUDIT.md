# Project-wide audit — 2026-11-26
## Scope and evidence
Baseline development branch 916b1f24b81f57feae4f9fb335affb4453150fa0, draft PR #1. Inspected repository tree, portable core, regression tests, tracker, MVP and architecture/hardware requirements. Local copies of all ten baseline core headers, thirteen regression programs and test script matched repository Git blob hashes. All 13 baseline programs passed using GNU g++ 14.2.0, C++17, strict warnings and fatal UBSan. Remote CI 45925704360 queued; no physical board access. Increment 14 adds two fictional diseases, inherited resistance, temporary immunity, death provenance validation and idle contact-scan avoidance. All 14 candidate regression programs passed under the same script; staged code hashes checked before commit. No release or complete hardware certification.

## Integrated observation
32 seeds 0–31, five simulated years each (216000 ticks per world, 6912000 ticks total); optimized host with fatal UBSan. Baseline and candidate each produced 352 descendant identities, 74 archived deaths, 278 living descendants at end. All 32 had births and living descendants at end. 31 of 32 had a child reach age two at a 600-tick observation. Living descendants ranged 2–10; total issued identities per seed 10–12. No adult expected at this short horizon. All steps accepted, final validation passed, no archive reached capacity. Candidate founder minimum health across runs 53; baseline 100. Repeated candidate with the two audit fixes yielded identical CSV metrics. This is five simulated years, NOT five real years equivalent. A 20-simulated-year eight-seed attempt reached execution limit after six complete seeds (0–5); all six reached adult descendants and ended with 28–37 living descendants. Do not claim remaining two completed. No full collapse recovery or same-seed canonical history replay certification. 74 deaths are all descendant deaths over the observation, not necessarily neonatal or infant deaths.

Reproduce:
```sh
bash scripts/test-host.sh
g++ -O2 -std=c++17 -Wall -Wextra -Werror -pedantic -fsanitize=undefined -fno-sanitize-recover=undefined -Icore tools/population_audit.cpp -o /tmp/genesis-audit
/tmp/genesis-audit 5 32 > population.csv
```
The audit tool logs seed and tick on rejection, and does not modify world state except through the actual step. Use longer runs and age/cause-specific statistics before tuning. CSV metrics are observational, not pass/fail population targets.

## Prioritized findings and required fixes
| Priority | Finding | Required closure |
|---|---|---|
| P0 | Lifetime history cap 512 stops all reproduction eventually even if mortal population collapses. Full history validated each second and transient arrays indexed by lifetime IDs. | Durable segmented archive/index and bounded live-slot scratch mapping; admission based on real storage availability; transactional checkpoint/append recovery and no ID reuse. Prove >512 lifetime identities and recovery from corrupt/truncated records. |
| P0 | No actual target firmware, active board configuration, display renderer, touch, storage, boot recovery or USB install path. Reference driver is inert. | Identify PCB/panel/flash/PSRAM/storage and backup path; safe disposable build, board-specific CI, renderer and 72h soak. |
| P0 | No save/load implementation, durable event journal or canonical serialization. Raw structs and host RAM are not a portable format. | Versioned encoding of all fields/RNG, dual validated checkpoints and crash recovery; verify migrations, same-seed canonical replay, no silent world reset. |
| P1 | Biological simulation only partially complete. First 32 seeds have births and nonzero populations but 74/352 descendants die within observation; causes and life-stage distribution need measurement. Founders at health 1 still act; no actual collapse/repopulation gate. | Record age/cause mortality and founder recovery; complete incapacitation, parental childbirth injury and accidents; multi-generation, high-density, seasonal shortage and collapse tests before tuning. |
| P1 | Disease model uses two provisional fictional types, bounded local exposure, temporary immunity, no strains/treatments/sanitation or pathogen evolution. Active-contact scans are quadratic worst case. | Measure exposure/recovery and immune diversity; shared medical/environmental mitigation, spatial indexing if benchmark requires, no forced outbreaks. |
| P1 | Founders and every citizen still rely on a homogeneous tile stock world; seasons only affect fixed renewal. No biome/weather, inventory, production, shelter, profession work or settlements. | Data-driven terrain, weather and conserved production/construction with bounded workloads. |
| P1 | No inventions, teaching, founder permanent knowledge, culture, founding event, encounter/rarity system, wonders, world effects or inspectable history implemented. | Existing approved MVP shared mechanisms in dependency order; do not create a separate engine for every event. |
| P1 | Replay tests cover individual slices, not every field/history across long runs and reload. Cross-platform equivalence, divergence and numerical pacing not validated. | Canonical comparison, controlled same-map and different-seed cohorts, measured thresholds, long scheduler horizons. |
| P1 | Host sizeof(World) increased from 114672 to 120832 bytes; not ESP32 footprint. Full-map renewal, fallback scans, care, validation and conception can spike latency. | Benchmark max active population, stack/heap placement, flash writes, render headroom and frame/tick budgets on exact board; do not allocate World on embedded task stack. |
| P1 | Remote CI queue unresolved. No device toolchain build. | Maintain local hash-verified test script; get actual remote runner result before merge; add firmware CI after hardware verification. |
| P2 | README and master tracker status stale; design notes mixed with completed code; core validation will grow too large. | README and gate summary corrected in this increment, keep immutable evidence and separate implementation status from approved design. Split validation/archives into modules before expanding. |
| Deferred | Full independent nations, diplomacy, advanced language, space exploration and livestream. | Preserve approved eventual scope; not needed for first primitive world release. |

## Delivery recommendation
Stop feature expansion. Finish biological acceptance and prioritize storage architecture in parallel with target identification; move durable-history work ahead of additional content because it is an absolute long-life blocker. Keep explicit disposable worlds; never initialize official world before the owner approves the completed release gates. No numerical balance changes made merely to raise survival statistics.

## Follow-up — increment 17
Natural density/resource regulation explicitly approved. First local crowding and acute stress mechanics implemented, 18 host regression executables passed. See POPULATION_EVIDENCE.md for research anchors and remaining calibration requirements. Does not close the P0 lifetime cap or P1 long-run balance findings.

## Follow-up — increment 21
Baseline remote portable CI 45931720560 passed. Added host checkpoint-bound archive retirement candidate and regression test, pending new-head CI verification. Lifetime population admission still blocked; no closure claimed until actual simulation above 512 and recovery verified. See ordered checklist in MASTER_TRACKER.md.

## Follow-up — increments 22–23
Host replay past the lifetime ceiling verified in CI 46011106294: 512 synthetic archived deaths followed by actual conception and delivery, with preserved identities. Automatic bounded archival cycle candidate added, pending exact new-head CI. P0 remains open at release scope: bounded-memory device index, real storage capacity/admission, physical durability, independent backup/UUID and sustained organic long-run tests.
