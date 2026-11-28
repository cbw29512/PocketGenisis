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
