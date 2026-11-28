# Integrated SD physical qualification — 2026-11-28
Owner-provided PowerShell and COM3 serial evidence. Firmware candidate checkout 3949ad3 (initial include-path fix 71859d8 compiled in 233.25s). Subsequent build/upload success 11.873s, static RAM 19476/327680, flash 643445/3145728, all upload hashes verified. ESP32-S3 rev0.2. Physical card mount and retained fixture separately verified on preflight branch commit 7a065ee.
## Actual civilization
Disposable world only, UUID 595cd572864b4544b42b78941b281581, population 2 throughout.
| Generation | Slot | Tick | Save microseconds |
|---|---|---|---|
|1|0|0|not captured|
|2|1|60|2368085|
|3|0|120|3324568|
|4|1|180|3321706|
|5|0|240|3323759|
|6|1|300|3321489|
Reported PSRAM free 8038027 throughout; internal free 310828 at creation, 310900 on generations 4–6. Stack watermark 51596 initially, 59296 on generations 4–6. Post-restart internal free 311048, stack watermark 62588.
```
rst:0x1 (POWERON),boot:0x8 (SPI_FAST_FLASH_BOOT)
POCKET GENESIS INTEGRATED SD TEST; NO OFFICIAL WORLD
WORLD_RESTORED generation=6 slot=1 tick=300 population=2 psram_free=8038027 internal_free=311048
WORLD_UUID 595cd572864b4544b42b78941b281581
WORLD_STACK_FREE 62588
```
Pass: integrated compilation/upload, explicit disposable creation, initial verified checkpoint, automatic advancement and alternating verified writes, normal power-on restoration of latest generation and identity. Still open: post-recovery autosave, interruption during active write, deterministic replay equivalence on target, mature population storage and memory peaks, combined display/storage, segmented history, reset adapter, 72h soak. No official world, no production certification, no merge. Keep physical candidate pinned during qualification.

## Post-recovery continuation — owner COM3 evidence
After restoring generation 6 at tick 300, same UUID and population 2, automatic saves continued:
| Generation | Slot | Tick | Save microseconds |
|---|---|---|---|
|7|0|360|3324363|
|8|1|420|3325626|
|9|0|480|3327054|
All three reported PSRAM free 8038027, internal free 310900, stack watermark 59296. UUID unchanged. Post-recovery continuation and alternating checkpoint writes passed. Eight completed automatic writes observed in total. This is short-run evidence only; peak memory, active-write interruption, deterministic replay, larger populations and soak remain open. Preserve the disposable world and pinned firmware; no official founding.

## Combined persistent observer — owner hardware evidence
Candidate 0c07803, build/upload SUCCESS 14.535s, all flash hashes verified, static RAM 24972/327680 and flash 718473/3145728. COM3 WORLD_AUTOSAVED generation=20 slot=1 tick=1132 population=2 psram_free=7269827 internal_free=296940, UUID 595cd572864b4544b42b78941b281581, stack watermark 62004, save time 3382802us. Owner explicitly confirmed visible map and tapping citizens opened their statistics. Pass: existing identity retained across upgrade, visible world, touch inspection, automatic save while graphical firmware active. Full combined power-on recovery log, long-run testing, peak memory and interrupted-write recovery remain open. Preserve this known-good firmware candidate.

## Accelerated observer qualification — owner COM3 evidence
Owner confirmed settings interface visible and incremental renderer ba76137 satisfactory. Candidate b61ae7b compiled/uploaded in 237.096s, all flash hashes verified; static RAM 50116/327680, flash 730533/3145728. Subsequent COM3 logs:
| Generation | Slot | Tick | Save microseconds |
|---|---|---|---|
|52|1|3588|3384128|
|53|0|4308|3385321|
Both saves report UUID 595cd572864b4544b42b78941b281581, population 2, PSRAM free 7269827, internal free 271696, stack watermark 57820. Tick delta 720 is consistent with the 12 ticks/s default pacing target per 60-second autosave interval; serial timestamps not captured, so exact wall-clock year duration is not certified. Existing world identity and alternating checkpoints preserved. Speed adjustment touch controls, reset cancellation/deletion, interrupted FAT reset, mature population and long-run memory/performance remain unqualified. Do not erase the running world for the overnight observation.

## Owner overnight report and photograph — 2026-11-28
Owner reported the physical device ran all night. Photo shows nominal 1h/year, Year 9, tick 350333, 10 living (two immortal founders, three infants, five children), zero current pregnancies, 10 archived deaths, nine discovered technologies and 12 completed structures. Global food 451100, water 409392, wood 296016, stone 152729, wild animals 15984, fish 3919. Verified save generation 533, tick 350044, Slot A, card capacity 62.5 GB. This establishes a much longer observed progression with births, deaths, discoveries, construction and a reported recent verified save; the photograph does not establish uninterrupted uptime, exact death causes, UUID, storage free space, memory peaks or 72-hour qualification. Do not invent cause counts or reset the world to collect them.

## Observer statistics increment
Candidate 228d76a1c4518be2ffbe541d7eebddb1c6b41847. Read-only demographics distinguish the immortal founders from mortal life stages, live/fatal births and lost gestations; Deaths tab shows six actual cause buckets plus unknown and paged resident identities with recorded age; Progress shows first discovered concept/year/founder and completed structure counts by type; Settings isolates brightness and existing two-stage nuclear option. SD capacity labeled accurately, no checkpoint schema or simulation rule changes. Current resident archive is complete only under existing fixed history layout; future external archive must replace its resident-only statistics with lifetime aggregates and paging. No permanent full event journal yet.

Direct git checkout failed DNS. Reconstructed six exact source files locally and confirmed remote Git blob hashes: observer_settings.hpp 7ae8d20b0930d47e577f3784a1f074c07b046e1a; observer_settings_test.cpp 78d8bf5252416dd8d573b10598fb8f9361c5022c; observer_population.hpp 675de1b7ecccffcda3334cdba09341ff3704c2d4; observer_population_test.cpp 5cb2fd75b0a0b337519bc1084b8ad535e75b81a7; world.hpp 9eb175038b685a2e218e72a7359599d36c769420; rng.hpp 890743463d9022d6a161b0f2d997a3315e0456e7. GNU g++ 14.2.0 strict C++17 -Wall -Wextra -Werror -pedantic -fsanitize=undefined -fno-sanitize-recover=undefined: settings test PASS, population projection PASS default and fatigue/reproductive-drive profiles (3 executed test binaries). Renderer, full regression/CI, target compilation, physical tabs, reset, actual cause extraction remain unverified. Do not flash during ongoing soak until candidate compiled and owner approves update; preserve existing card/checkpoint.
