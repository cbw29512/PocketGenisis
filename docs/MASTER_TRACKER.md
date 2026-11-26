# Master tracker
| Gate | Status | Exit criteria |
|---|---|---|
| G0 MVP/schema | Review | Owner approves baseline and finalized schema |
| G1 hardware | Blocked | Exact PCB and safe USB backup/flash path verified |
| G2 core | In progress | Initial RNG, seeded resource grid, founder traits and clock host-tested; full transition loop and state fixtures pending |
| G3 citizens | Pending | Lifecycle and founder recovery tests |
| G4 environment | Pending | Weather/resource conservation/regeneration |
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
Work strictly from MVP and this tracker. Each increment closes a named acceptance condition or records the precise blocker; no new feature work or livestream work. Use development branches, regression tests, verified CI before merge. Hardware gate blocks drivers/flashing but not portable host work. Next: formalize citizen needs and local resource transactions, implement bounded action loop with conservation tests; then lifecycle and environmental mechanics. Initial host test passed locally; remote CI and full emergence gate not certified.
