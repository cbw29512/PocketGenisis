# Elecrow CPU/PSRAM probe (not production firmware)
Owner's serial diagnostics confirm ESP32-S3 revision 0.2, 4 MiB quad flash, 8 MiB embedded PSRAM, COM3 via CH340; two independent complete factory reads matched SHA256; secure boot and flash encryption disabled. Keep the backups off GitHub and separate from other boards. Exact PCB revision remains unknown. Generic S3 N4R8 build configuration based on Elecrow's qio_opi reference; no display, touch, SD, radio, GPIO or persistent world logic. No eFuse commands. Build before authorizing any upload. Upload replaces factory firmware; restore requires verified correct-device backup and separate authorization. No official world creation.
Run from repository root: pio run -d benchmarks/elecrow
The sketch allocates one World in PSRAM, logs free/largest internal and external heap blocks, then measures 600 real step_world calls per synthetic population 2/32/64/128/256 plus crowded 256 with an infectious founder. Includes renewal and conception cadence; initial/final counts may differ. Reports mean/max microseconds excluding deliberate yields. This is a bounded CPU/heap probe, not a final concurrent rendering, SD, or long-duration test. Host tests exercise the same fixture; on-device compile and results still required. Suggested max tick budget 200 ms, with remaining time for rendering/storage; actual integrated worst-case needs later measurement. No migration or durability certification.
References: https://github.com/Elecrow-RD/CrowPanel-7.0-HMI-ESP32-Display-800x480 ; https://github.com/platformio/platform-espressif32

## Scale experiment
Previous physical COM3 run: 256 ordinary citizens mean 1845 us/max 3634 us; crowded infectious-founder case mean 1935 us/max 9669 us; internal free 372340 bytes and PSRAM free 8265179 bytes during both cases; benchmark completed. These are CPU-only results supplied by owner.
Build with `pio run -d benchmarks/elecrow -e elecrow_s3_scale_probe`. The separate environment defines PG_DISPOSABLE_SCALE_PROBE, 2050 total active slots (two spare at 2048 so conception is not categorically suppressed), 4096 resident history slots, and a 32 KiB Arduino loop task stack to accommodate expanded scratch arrays. Production/default capacities and rules are unchanged; no storage codec is linked. Test 256/512/1024/2048 in both ordinary and initial crowded-infection scenarios using 600 authoritative ticks per case. Log progress, actual population, minimum population, worst/mean latency, count exceeding 200ms or one second, minimum free task stack and free/largest heaps. No assertions that all 2048 survive: initial population is a load fixture, not demographic calibration. Larger capacity changes scratch overhead, and the extra slots allow conception; results need not exactly match the original 256-only firmware. If compilation succeeds, explicitly upload this environment only to confirmed COM3. No official world, SD or display access. Host fixture test uses 120 ticks crossing disease and conception boundaries; device build and benchmark are separate acceptance gates. A passing CPU case requires all steps valid and maximum tick <=200ms, no crashes, safe stack/heap headroom; graphics, storage, long-run and population sustainability remain unqualified.

### Xtensa large-initializer build correction
The first scale build on Windows compiled but failed at link with `l32r: literal target out of range` in setup (no upload). Default-construct large arrays in place only in the disposable scale configuration, move placement construction into a non-inlined helper, and enable Xtensa `-mtext-section-literals` in the scale environment as recommended by the linker. Host regression checks default element values. Core tick logic and production capacity unchanged; the revised device link and runtime remain to be verified. https://gcc.gnu.org/onlinedocs/gcc/Xtensa-Options.html

## Measured scale baseline and profiling
Owner's successful corrected build/upload and serial run on COM3: dca5e03, 600 ticks each. All eight cases completed, zero deaths and no count reductions. Normal/crowded means and maxima in microseconds:

| Population | Normal mean/max | Crowded mean/max | Normal/crowded >200ms | Normal/crowded >1s |
|---|---|---|---|---|
|256|2744/26157|2839/32527|0/0|0/0|
|512|8143/122773|8413/133611|0/0|0/0|
|1024|17538/793782|18947/845298|1/1|0/0|
|2048|37936/3455729|59002/3753356|1/10|1/7|

Heap during each case: internal free 347716, largest 303092; PSRAM free 7756067, largest 7733236. Minimum stack free reported 17284 bytes; PSRAM returned to 8386035 on release. CPU target met at 512, not 1024 or 2048; no replacement hardware justified until optimization and integrated tests. Same scale environment now adds five phase timings (survival includes validation/care/search), each mean/peak and contribution to the overall worst tick, plus absolute worst tick. Timer reads add small measurement overhead, no serial inside timed regions. The shared observed tick retains authoritative order, production and RNG. Default no-op observer preserves standard calls and rules. No spatial algorithm changes yet: profile first, then optimize measured hotspots with replay parity. Host tests check canonical state and phase order; physical rerun required. No official founding.

## Measured phase bottlenecks and optimization candidate
6e15595 profiler physical run completed. At 2048 crowded: overall worst 3750044 us, lifecycle peak 3434791 us, disease peak 2228077 us (different ticks), survival/validation peak 28703 us, mortality 3711 us, renewal 2917 us; 7 ticks >1s. Normal worst 3452931 us with lifecycle 3416925 us. Worst tick 864600 in both. Free PSRAM 7756067, internal 347716, minimum stack 17028 bytes. Host CI 46067449414 passed. Candidate: bounded sorted tile multisets for pulse-snapshot disease counts; packed eligible parent tile index, clipped row ranges and ascending candidate bitset for original reservoir order. No RNG/rule/checkpoint changes, no new persisted fields; existing valid state required. Disease O(n log n) bounded neighbor counting, conception O(n log n + local eligible pairs) with same RNG draws (dense case can remain expensive). Separate frozen reference parity test. Device stack scratch increases; verify high water mark, compile and re-run identical 600-tick cases before setting capacity. No 4096 test yet.

## Separate 4096-citizen acceptance probe
Owner approved 4096 active citizens as the engine performance target, not a gameplay cap. Build `pio run -d benchmarks/elecrow -e elecrow_s3_4096_probe` after the optimized 2048 baseline is physically verified. This isolated configuration has 4098 live slots and 8192 history slots, a provisional 64 KiB loop stack, and ordinary/crowded 4096 initial-population cases (600 ticks each). The ordinary fixture expands its grid only above 2048; existing baseline coordinates and capacities remain identical. Existing natural disease and resource behavior stays enabled; no new global population pressure is introduced. Target <=200 ms maximum tick, <50 ms disease peak, <100 ms lifecycle peak, valid state, zero crashes, and adequate heap/stack headroom. Dense eligible-pair reservoir selection remains quadratic to preserve existing RNG draw order; do not claim 4096 performance until measured. Device build, RAM and stack allocation, and physical run remain unverified. Production capacity, save schema and history durability are unchanged; integrated rendering/storage and long-run history still require separate acceptance tests.

## Wildlife experiment
Build `pio run -d benchmarks/elecrow -e elecrow_s3_wildlife_probe`. Disposable 2048/4096 ordinary/crowded cases, 600 ticks each. Separate wildlife PSRAM sidecar; combined RESULT and LIMITS include wildlife time, existing PHASE and WORST_TICK lines still describe only the human core; WILDLIFE reports separate peak and totals. See docs/WILDLIFE_CONTRACT.md. Physical compile/upload/run not verified. No save changes or official world.

## Physical wildlife baseline — owner serial COM3, a03627f, 2026-11-27
All four 600-tick cases completed; no crash, no >1s tick. Initial animals 7838 each, sidecar 96000 bytes. Times in microseconds:

| Humans/contact | Mean/max combined | Lifecycle peak | Disease peak | Wildlife peak | Final humans/animals | Animal births/deaths/hunts/meals | >200ms |
|---|---|---|---|---|---|---|---|
|2048/0|35054/85048|39749|5592|10089|2048/8541|1691/988/0/0|0|
|2048/1|35191/115085|60902|30241|10101|2048/8257|1569/1150/6/10|0|
|4096/0|69039/164063|85322|11110|12155|4096/7705|1436/1569/0/0|0|
|4096/1|71157/351037|245484|61239|12109|4080/6992|1229/2075/184/428|1|

All cases: minimum free stack 35252, internal free 314948/largest 270324, PSRAM free 7078419/largest 7077876. Released PSRAM 8386035. Predation 208 per case; population accounting holds. Host CI 46069238777 passed. 4096 crowded fails max budget; lifecycle and disease remain priority. This is short disposable CPU evidence, not demographic calibration or production certification. Candidate next optimization: avoid unnecessary RNG rejection-threshold division, reuse consecutive parent-tile masks, cache disease neighborhood counts. Preserve exact draws, pair order and canonical state. Same probe required for validation.

Wildlife movement revision: same four cases now include food-directed one-tile migration and log WILDLIFE_MOVEMENT. The explicit land-use mask starts undeveloped for these cases; city exclusion is exercised by host fixture tests, not this device run. New ecology behavior changes demographics intentionally; compare latency with the prior physical baseline, not identical animal totals. No official save change.

## Accelerated discovery test (separate firmware)
Build `pio run -d benchmarks/elecrow -e elecrow_s3_discovery_probe`. Runs 3600 actual disposable world ticks, starting from two founders, with accelerated 60-tick experiment opportunities, reporting RESEARCH, CIV_PROGRESS and DISCOVERY_RESULT over serial. This is a founder-only research transaction test, not a production clock or calibrated progression forecast. Does not run the wildlife benchmark, render graphics, or write saves. Existing probe environments unchanged. See docs/DISCOVERY_CONTRACT.md. CI/device verification pending.

## Construction prototype
`elecrow_s3_construction_probe` runs 43200 disposable seed42 ticks with research, local construction and ordinary reproduction. BUILD events and CONSTRUCTION_RESULT report completed sites and births. No display/save/functional building benefits. Existing family/discovery probes unchanged. CI/device pending.

## Host world diversity study
Default CI runs 16 seeds x 3600 ticks via tests/civilization_diversity_test.cpp. A larger optional study on a C++17 host: `g++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic tests/civilization_diversity_test.cpp -o diversity` then `./diversity 1000 43200 > diversity.csv`. This can take substantial CPU time; begin with 16 seeds. CSV includes actual discovery order (A fire, B stone tools, C shelter, D cooking, E storage), completed structure tile layout, births, living count and timing. Final DIVERSITY line summarizes distinct observed outcomes, stalled worlds and first birth range; zero means not observed, not a guaranteed future outcome. Uses accelerated research, ordinary lifecycle, no production saves. Smoke test is not a full long-term calibration. No ESP32 flash needed for host study.

## Functional food store probe
`elecrow_s3_storage_probe` extends construction for 43200 ticks with actual local deposits, shortage fallback and spoilage. `STORAGE_RESULT` reports balance, deposits, withdrawals, spoiled and bytes. Controlled host tests demonstrate scarcity; no forced shortage in hardware probe. No official world or save. CI/device pending.

`elecrow_s3_cooking_probe` extends storage with local campfire cooking. COOKING_RESULT reports meals, fuel, extra hunger relief and bytes; absence of actual cooking in an unforced world is reported. Controlled host tests exercise benefit. CI/device pending.

`elecrow_s3_shelter_probe` extends cooking with local bounded shelter recovery. SHELTER_RESULT counts actual extra recovery; zero can occur naturally. Host fixtures exercise recovery and mortality guards. No official world, rendering or saves.

`elecrow_s3_settlement_probe` extends shelter with local viable home-return policy; SETTLEMENT_RESULT reports steps and arrivals. No forced settlement or official save; host tests and device run pending.

`elecrow_s3_housing_probe` extends settlement with demand-driven extra shelter slots, same material and labor rules; HOUSING_RESULT reports actual huts and unmet planning demand. Earlier profiles retain original capacity. No official save. CI/device pending.

`elecrow_s3_agriculture_probe` extends housing with one actual prepared field and seed/water/time/labor crop cycle. AGRICULTURE_RESULT reports observed planting, tending, harvest and output; absence of harvest is observable, not forced. Controlled host test verifies mechanics. CI/device pending.

`elecrow_s3_irrigation_probe` extends agriculture with demand-gated adjacent channel and conserved water redistribution. IRRIGATION_RESULT reports actual completed channel and water moved; zero demand is allowed. Host fixture verifies transfer in a real tick; CI/device pending.

`elecrow_s3_multi_field_probe` extends irrigation with four independent field slots; existing AGRICULTURE_RESULT is field zero, MULTI_FIELD_RESULT reports aggregate completed fields and harvests. CI/device pending.
