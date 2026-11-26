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
