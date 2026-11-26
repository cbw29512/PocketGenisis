# Portable world contract — draft v1
Canonical interchange: UTF-8 JSON; versioned records, fixed-point units, stable enum IDs; unsigned 64-bit IDs/ticks/RNG words as decimal strings. Compact device encoding allowed if losslessly convertible. All IDs scoped to immutable world UUID. No raw C struct dumps. Unknown required versions fail closed.
| Record | Required logical fields |
|---|---|
| World | schema_version, rules_version, world_id, seed, rng_algorithm, rng_state, creation_utc nullable, tick, next_citizen_id, next_entity_id, next_event_id, dimensions, settings, founder_ids, last_committed_event_id |
| Terrain | x, y, biome, elevation, fertility, moisture, stocks, regeneration_state, building_id nullable |
| Citizen | citizen_id, name, parents, founder_flag, birth_tick, apparent_age, life_stage, primary_purpose nullable for children, traits, skills, health, hunger, thirst, fatigue, position, action, progress, household_id, settlement_id, inventory, knowledge_ids, pregnancy nullable, alive |
| Pregnancy | parent_ids, conception_tick, due_tick, status |
| Relationship | citizen_ids, relation_type, affinity, updated_tick |
| Building | entity_id, kind, position, builder_ids, progress, condition, inventory, settlement_id |
| Settlement | entity_id, name, founded_tick, territory, inventory, status |
| Technology | technology_id, rules_version, prerequisites, costs, minimum_effort, progress, discovered_tick nullable, discoverer_ids, known_by_ids, record_locations |
| Environment | season, weather, remaining_ticks, water_cycle, ecosystem_state |
| Event | event_id, tick, type, subject_ids, related_entity_ids, payload, rules_version |
| Archive person | citizen_id, name, parents, birth_tick, death_tick, cause, purpose, traits, accomplishments, settlement_history |
| Checkpoint | schema_version, rules_version, world_id, sequence, tick, file_lengths, checksums, event_cursor, encoding |
## State transitions
Citizen infant -> child -> adolescent -> adult -> elder -> deceased; founders physically adult forever, may be incapacitated -> recover, never deceased. Death and archive logically atomic. Pregnancy none -> gestating -> birth/loss -> recovery -> none. Technology unknown -> eligible -> experimenting -> discovered -> maintained/forgotten; immutable discovery history. Building planned -> under construction -> complete -> damaged -> repaired/ruined.
## Invariants
Exactly two immortal founder identities. No reused IDs, negative resources, invalid positions or dangling parent references. Personality traits 0–100 (curiosity, diligence, compassion, creativity, sociability, risk tolerance). Seeded inheritance and variation. Birth budget checked before creating citizen; low population recovery respects capacity. Monotonic simulation clock; no automatic outage catch-up. Persist RNG and rules version. Same seed and rules replay deterministically. Finalize field ranges, units and golden fixtures before code.

## Seed and replay contract
Persist canonical seed, RNG algorithm/version, complete state for each named random stream, generator version, initial settings, rules version and tick. Derive independent streams using a documented stable algorithm (terrain, traits, environment, decisions, reproduction, research); no wall clock or device ID in simulation RNG after initialization. Generate a fresh seed on new-world initialization unless explicitly supplied for testing. Save/reload preserves streams exactly. Canonical state comparison excludes external metadata but includes all simulation-relevant state and event payloads; stable iteration and fixed-point units prevent platform drift. Stream algorithm and fixtures must be finalized before implementation.

### Increment 5 implementation subset
Citizen adds parent_ids, birth_tick and stable purpose enum (0 none, 1 gatherer, 2 builder, 3 inventor); life stage derived from age at current tick, founders always adult. World holds founder-pair due_tick (0 means no gestation), descendant_count and next_citizen_id. This bounded active population is not yet an archive or portable save representation; later death handling must decouple next ID from active count and preserve parent references. See CORE_CONTRACT.md for provisional rules.

### Increment 6 implementation subset
Citizen health (0–100), lifespan_years (60–80 for descendants, provisional fixture default 70; ignored for founders). Death record stores the complete citizen snapshot, death_tick and stable cause enum (1 starvation, 2 dehydration, 3 old_age). Active descendants and archive are disjoint; monotonically issued IDs, with no gaps or reuse. Fixed provisional lifetime capacity 512 reserves archival space before conception; permanent on-disk archive remains a release blocker.

### Resilience and exceptional-project contract (design, not implemented)
Hazard catalog: stable hazard ID, trigger, region, baseline probability in integer basis points, duration, exposure, effect primitives, mitigation rules and rules version. Mitigation uses measurable local capability, condition, coverage, knowledge and stock; clamp probabilities and effects; no era-wide multiplier. Persist active hazard instance IDs, start/end ticks, affected region, resolved effects and associated event IDs when implemented.
Exceptional projects extend Building (not a separate construction engine): project kind/catalog ID, site, proposal tick, proposer/contributors, prerequisites, reserved/consumed stock, labor progress, condition, completion tick, milestone event ID. A natural landmark references a stable terrain region and discovery event, not a construction record. History records actual event transitions exactly once; loss of a building or knowledge must not erase milestone history. Final fixed-point formulae, enum IDs, ranges and replay fixtures must precede implementation.

### Increment 7 implementation subset
Replace founder_due_tick with Pregnancy {parent1, parent2, due_tick} (all zero when idle); lost_gestations is a saturating cumulative counter, not a substitute for the eventual event journal. Genealogy permits any valid pair of mortal parents (or the founder pair). Parent identity precedes child ID, birth chronology is validated, and death archives retain references. One concurrent gestation is a provisional host implementation limit.

### Exceptional encounter pacing (design only)
Encounter catalog: stable type ID, rarity tier, contextual eligibility and candidate weights, effects, rules version; opportunity state: simulation-tick cadence, last evaluated opportunity, last generated rare-or-higher encounter tick (nullable), dedicated versioned RNG stream/state or explicitly isolated substream. Persist any active instance and its event identity. Use integer weights and bounded probability draws, stable order, no elapsed-wall-time catch-up, and no retries on render/reboot. No guarantee of rare events. Generated encounter pacing is distinct from the physical discovery of preexisting deposits, historical milestones, normal hazards and earned achievements. Final IDs, rates and cooldowns require tests and approval.

### Founding event and discovery influence (design only)
World origin record: origin_catalog_id/version, immutable founding_event_id, applied_at_tick=0, affected entity IDs, effect payload, initialization transaction state. Persist with world UUID and journal; no fallback recreation on failed load. Origin stream/substream derivation must preserve existing stream identities or explicitly version the rules. Discovery influence record: source discovery/event ID, target catalog ID, required active capability/resource/artifact, bounded fixed-point weight modifier or eligibility predicate, scope, optional expiration/consumption, rules version. Derive active modifiers from actual valid world state, not an accumulated global luck value. Deduplicate by source and target; use stable order, explicit caps and independent no-event outcome. Store enough provenance for deterministic replay and historical inspection. Final numeric rates, effect identifiers and fixtures precede code.

### Exceptional milestone presentation (design only)
Extend Event with stable significance tier, catalog/version, location/entity references, causal_event_ids, observed effect summary fields and optional follow-up event links. Distinguish known facts from potential outcomes and undiscovered data. UI presentation cursor/acknowledgement/quiet-mode preferences are separate from canonical simulation state and RNG. Derive announcement styling from tier; retain journal records when transient UI queue is full. Reopening a historical event must not apply effects, advance RNG or duplicate event IDs. No new simulation primitive solely for a visual effect.

### Shared world effects (design only)
Effect definition: stable effect_kind_id, target attribute/transaction primitive, operator and fixed-point magnitude, scope, eligibility/maintenance predicate, duration policy, stacking group, bounds and rules version. Active effect instance: stable effect_instance_id, source event/entity/discovery ID, affected target or region, start_tick, optional end_tick, condition/source reference, lifecycle status and applied transaction marker if applicable. Derive effective parameters from authoritative base state plus eligible effects using versioned deterministic order, deduplication and caps. Persist source links, duration and state; no observer/renderer RNG; no reapplying one-shot effects on load. Historical event records remain after active effects end. Avoid redundant modifiers for existing authoritative resource, health and knowledge state. Final operators, units, caps, IDs and fixtures must be specified before implementation.

### Increment 8 implementation subset
Append Action enum care=4; previous 0–3 values unchanged. Infant dependency derived from birth tick and parent IDs. Care requests are transient per-tick state, not new save fields. The infant's eat/drink records one consumed unit; parent's care is not an additional consumption. No independent infant foraging. Orphan adoption/households and final care balance pending.
