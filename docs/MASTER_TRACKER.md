# Master tracker
| Gate | Status | Exit criteria |
|---|---|---|
| G0 MVP/schema | Review | Owner approves baseline and finalized schema |
| G1 hardware | Blocked | Exact PCB and safe USB backup/flash path verified |
| G2 core | In progress | Initial RNG, seeded resource grid, founder traits and clock host-tested; full transition loop and state fixtures pending |
| G3 citizens | Pending | Lifecycle and founder recovery tests |
| G4 environment | In progress | Seasonal renewable stock slice CI passed; weather, terrain suitability and balance remain |
| G5 technology | Pending | Prerequisites, failure, teaching and loss tests |
| G6 saves | Pending | Corruption recovery and migration |
| G7 target | Pending | Board-specific CI, display/touch/storage |
| G8 integration | Pending | Render and inspect citizens, measured capacity |
| G8a seeded emergence | Pending | Reproducibility, save/reload and multi-seed divergence suite with controlled same-map cohort; thresholds agreed before release tests |
| G9 reliability | Pending | Five-year equivalent accelerated test and 72-hour hardware soak |
| G10 release | Pending | All gates passed; owner confirms hardware and foundational ruleset |
| G11 official founding | Pending | Explicit owner approval; initialize new official world, record ID and creation date; verify backup |
| S1 optional streaming | Deferred | After G11 and owner approval; read-only feed, disconnect isolation, broadcast test |
No unverified merges or destructive flashes. Later eras and migration are roadmap.

Development saves are disposable and may be explicitly reset; official saves must be isolated and preserved. No streaming implementation before civilization release.

## Development guardrail
Work strictly from MVP and this tracker. Each increment closes a named acceptance condition or records the precise blocker; no new feature work or livestream work. Use development branches, regression tests, verified CI before merge. Hardware gate blocks drivers/flashing but not portable host work. Next: formalize citizen needs and local resource transactions, implement bounded action loop with conservation tests; then lifecycle and environmental mechanics. Initial core remote CI passed. Increment 2 adds founder needs, local movement and consumption; both remote CI runs passed. Full emergence gate not certified.

Pinned upstream references: see UPSTREAM_TRACKER.md. Next after survival CI: renewable resource recovery and bounded search; then common citizen lifecycle, construction and inventions. Do not claim G2 complete from this slice.

Increment 3 adds deterministic seasonal renewable production and resource accounting tests. Next: verify CI and implement bounded resource search beyond adjacent tiles; then common citizen lifecycle. No full ecological or emergence certification yet.

Increment 4 adds bounded resource search and an isolated MIT upstream hardware reference; CI pending. Next after CI: common citizen lifecycle state and tests. G9 wall-mounted acceptance: unattended boot/recovery, power, ventilation and temperature, dimming without pausing world, accessible service ports; exact device and final enclosure tests required. Do not certify permanent founder recovery from bounded search alone.
