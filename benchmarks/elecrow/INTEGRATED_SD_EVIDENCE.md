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
