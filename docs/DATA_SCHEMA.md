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
