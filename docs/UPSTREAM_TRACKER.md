# Pinned upstream references
Use upstream URLs at immutable revisions rather than duplicating entire histories in public branches. No upstream code copied in this increment. Repository-level SPDX metadata checked via GitHub; file-level copyright, dependencies and compatibility must be audited before any import. Retain all applicable notices.
| Upstream | Pinned commit | Reported license | Candidate |
|---|---|---|---|
| [ESP32 ecosystem](https://github.com/ootake0914-dotcom/esp32-ecosystem-sim/tree/e007643d066c84a238c267fa5108d7b1cbfedcb5) | e007643d066c84a238c267fa5108d7b1cbfedcb5 | MIT | Survival, reproduction, ecology |
| [The Human Box](https://github.com/1homsi/thehumanbox/tree/02e32e5b096532a842983f64f3a94689ba690ba1) | 02e32e5b096532a842983f64f3a94689ba690ba1 | MIT | Occupations, tech, economy |
| [ESPLAY32](https://github.com/aerodynamics-py/ESPLAY32/tree/b73b5705ea62534eb59303721ccb815825629967) | b73b5705ea62534eb59303721ccb815825629967 | MIT | Display and touch; board verification required |
| [HistSim](https://github.com/djeyprott/sim/tree/2cec198c5464414c70c7b74bb2ce26762b20152e) | 2cec198c5464414c70c7b74bb2ce26762b20152e | Apache-2.0 | Climate and historical systems |
## G2 survival
Inspected CYD ecosystem source cyd-version/cyd-ecosystem-sim.ino: entities have energy, seek resources, consume them and expend energy. Its floating-point agent positions and energy do not directly fit our fixed-point, deterministic tile contract. Implement an original bounded integer local-needs transaction; no code imported. Later audit reproduction and regeneration independently. Do not merge upstream wholesale.

## G2 resource search
Read pinned ecosystem CYD source around lines 473–504: bounded nearest active plant selection and movement toward target. Adapted the general nearest-resource mechanic to Manhattan tile rings, deterministic integer movement and authoritative stock; no ecosystem code copied.
## G7 actual selected upstream copy
Copied ESPLAY32 LGFX_Config.h, build settings (inert .reference file) and MIT LICENSE into reference/esplay32, exact pinned revision. This is isolated from builds until board identification, dependency audit and verification. No other upstream code imported yet.

## G3 lifecycle
Read pinned ecosystem MIT LICENSE and CYD source lines 192–225, 549–567 (spawn with inherited variation, energy-driven reproduction and death). Adapted conceptual bounded mutation and condition-driven reproduction into two-parent integer traits, explicit gestation and monotonic identities. Upstream reuses inactive slots and debits energy before confirming allocation; not suitable for permanent genealogy/capacity safety without changes. No upstream ecosystem source copied. Existing ESPLAY32 reference untouched.

## G3 mortality
Re-read pinned ecosystem CYD source lines 529–570 and MIT license: depletion causes death but upstream discards identity. Adapted depletion concept with deterministic health and retained genealogy; no source copied.
## Additional game references (pinned, not imported)
- LibColony MIT: https://github.com/mafik/libcolony/tree/7406bd9fbb00e53273ea9da84eeba5aa26af382f — inspected README, license and header; candidate task scoring, benchmark stack use and complexity; global optimal assignment must not override fixed citizen roles or entropy.
- Unciv MPL-2.0: https://github.com/yairm210/Unciv/tree/af2bdcf467ac325979f95eacf221846874b93472 — technology dependencies, design reference until file-level license review.
- Freeciv GPL-2.0: https://github.com/freeciv/freeciv/tree/4d15f07706d5f6a0c6dd60a1337b4ef9eedd61a1 — design reference only, no source import.
- Widelands GPL-2.0: https://github.com/widelands/widelands/tree/ed17072e0203b0718e882dc2acc686db0dab3a4d — production/construction design reference only.
- Open Mars Colony Simulator MIT: https://github.com/AdamCofala/Open-Mars-Colonization-Simulator/tree/b89152bd40e6561e2d93a29e720271e21bcbd2fa — candidate colonist mechanics.
Use universal data-driven work, recipe, invention and construction systems within approved MVP. Proprietary games are conceptual references only.

## G3 multigeneration
Re-read pinned ecosystem MIT license and CYD spawnHerb lines 192–225: bounded slot allocation and inherited mutation. Reused our existing two-parent mutation/lifespan primitive rather than importing single-parent floating-point spawning or slot identity loss. New deterministic pair selection and archive validation are original code. No new third-party code copied.

## G3 dependent care
Re-read LibColony pinned MIT license and header design lines 15–65 (needs/work priority, distance and autonomy). Reused the concept of priority-based task selection with bounded local parent requests; no optimal assignment engine or upstream code imported. All consumption routes through one authoritative resource transaction.

## G3 recovery
Re-read pinned ecosystem MIT license and CYD source lines 473–504, including nearest-resource search and random wandering when none found. Retain deterministic bounded local search; original founder-only full-map fallback and reunion rule avoid indefinite local starvation or isolation without importing upstream floating-point behavior. No code copied.

## G3 concurrent gestation
Reused increment 7's original deterministic reservoir pair selection, inheritance and archive reservation; expanded bounded gestation storage and validation, no new upstream dependency or source copied. Earlier pinned ecosystem reference is not a multi-parent pregnancy implementation and is not substituted for our existing mechanics.

## G3 local orphan care
Reuse existing validated live-citizen lookup, local care requests, compassion trait, shared consumption and dependent movement. No new third-party source or dependency required.

## G3 persistent guardian
Reused local orphan-care selection, authoritative resource consumption and existing genealogy/archive index; no imported source or new dependency.

## G3 newborn health
Reused existing seeded RNG, birth inheritance, health recovery and archive primitives; no imported source or dependency. Provisional risk model is original simulation tuning.

## G3 disease and audit
Original fictional two-slot model reuses existing health, seeded RNG, inheritance and death archive primitives; no new third-party source or dependency imported. Population audit uses existing portable engine without observer mutation.

## G6 checkpoint foundation
Original field-wise codec and POSIX adapter using C++17 standard library and operating-system file primitives; no new imported source or dependency. Reuse existing world validation and simulation replay.

## G6 live-slot decoupling
Reuse existing original stable ID sorted-array lookup; no third-party code or dependency needed for this internal refactor.

## G6 immutable death batches
Reuse our existing field visitor, CRC, validation and host file primitives. Original bounded segment codec and create-only publication; no new upstream dependency or copied source.

## G6 historical identity reference index
Reuse original shared genealogy checks, canonical death codec and validated immutable host publication. C++17 standard map is a host reference only; no imported source/dependency or claim of embedded performance.

## G6 checkpoint-bound retirement
Reuse original canonical field visitor, CRC, immutable batch publication, historical identity index and POSIX alternating checkpoint publication. No new third-party dependency or source imported.

## G6 automatic archive cycle
Reuse original host recovery, immutable publication, transactional retirement and bounded deterministic replay. No imported source or new dependency.

## G6 bounded disk identity lookup
Reuse canonical field codec, immutable segment loader, shared genealogy and combined validation. Original POSIX scratch-row adapter; no third-party source or dependency.

## G3 founder recovery
Reuse original health floor/healing, conserved need consumption, cardinal movement, founder reunion and reproduction eligibility. No imported code or new dependency.
