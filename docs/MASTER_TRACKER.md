# Master tracker
| Gate | Status | Exit criteria |
|---|---|---|
| G0 MVP/schema | Review | Owner approves baseline and finalized schema |
| G1 hardware | Blocked | Exact PCB and safe USB backup/flash path verified |
| G2 core | In progress | Initial RNG, seeded resource grid, founder traits and clock host-tested; full transition loop and state fixtures pending |
| G3 citizens | In progress | Founder gestation, maturation, health, basic mortality and bounded RAM archive implemented; CI pending; durable archive, disease, accidents, multigeneration reproduction implemented but unverified; care and recovery remain |
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

Increment 4 adds bounded resource search and an isolated MIT upstream hardware reference; both CI runs passed (45896118998, 45896113059). Next after CI: common citizen lifecycle state and tests. G9 wall-mounted acceptance: unattended boot/recovery, power, ventilation and temperature, dimming without pausing world, accessible service ports; exact device and final enclosure tests required. Do not certify permanent founder recovery from bounded search alone.

Increment 5: actual founder-pair gestation, unique descendant IDs, inherited traits, common survival and life-stage/purpose transitions, bounded active capacity. CI pending. Next: health, death and lossless historical archive handoff before slot reuse, then descendant reproduction and parental care; recovery and population balance remain unverified. Do not certify G3 from this slice.

Increment 6: health depletion/recovery, old age and need-related death, stable death records and no ID reuse. Prior lifecycle CI still queued at review; run full suite on new head. Fixed 512 lifetime ID admission ceiling is temporary; permanent archival storage must remove this before production, not drop history. Next: verify CI, generalize descendant reproduction and care, then remaining health causes and founder recovery. No G3 certification yet.

## Owner-approved resilience and wonders
G3/G4/G5: test actual capability-based disease, famine and weather impact mitigation and regression when infrastructure or knowledge is lost. Natural hazards must not become impossible with progress. G5/G8: after ordinary construction, implement minimal exceptional-project proposal/completion via existing construction, discovery and event records; no bespoke wonder engine. Additional wonder content and advanced hazards deferred. See MVP.md and DATA_SCHEMA.md. Requirements only; no implementation or new test claim in this update.

Increment 7: founder or adult-descendant gestation, bounded seeded pair selection, actual two-parent inheritance, archived genealogy validation and gestation cancellation when a parent dies. Regression suite added. Prior lifecycle/mortality/MVP CI still queued at audit (45908278244, 45909440919, 45910659180); local clone unavailable (DNS). No merge or test-pass claim. One concurrent gestation is provisional; parallel family pregnancies, parental care, disease and founder recovery remain. Next verify all host tests, then parental care and health events. Resilience and wonders stay approved in MVP, not implemented ahead of prerequisites.

## Encounter balance requirement
G4/G5/G8a/G9: calibrate optional contextual rare encounters independently from ordinary milestones, resource discovery and actual achievement. Earlier short mean intervals are superseded by decades-scale real-time-equivalent targets; the 90-simulated-day lower-bound pacing fixture and all rates remain provisional; validate zero-event worlds, short-window clustering, population scaling and deterministic save/reload before final numeric approval. No pity timer, forced rarity or artificial story director. Culture/rare materials use the same resource, creation, knowledge and journal systems. Documentation only; no encounter implementation or test-pass claim. Maintain current G3 priorities.

## Founding event and causal rarity update
G2/G6/G8a: exactly one deterministic origin event applied atomically on explicit new-world creation; test same seed, different origin seeds, no reapplication on save/load or collapse, immutable provenance and founder invariants. Implement small catalog via supported core mechanics, not expanded alien/space gameplay. G5/G8a/G9: discovery influences must be causal, bounded, reversible where capability lost, and unable to guarantee a later rare outcome; compare controlled eligibility and weight changes with aggregate chance caps. Test rarity tails and incomplete catalogs across 1–50 real-year-equivalent isolated simulations; full civilization long-run gate remains at least five real-year equivalents before release, with longer scheduler tests not a claim of physical decades of uptime. No completion requirement or guaranteed collection. Documentation only; maintain G3 development order and verify pending CI.

## Exceptional milestone impact and presentation
G5/G8/G8a: rare/epic/legendary catalog entries require meaningful causal world impact or opportunity; test distinct tier styling, unique durable event IDs, journal links, replay of missed events, quiet mode and no observer-induced simulation changes. Legendary is a major historical milestone, not guaranteed beneficial or a scripted outcome. Use shared read-only presentation path; no separate cinematic engine. Requirements only; no implementation/test-pass claim; current G3 priorities unchanged.
