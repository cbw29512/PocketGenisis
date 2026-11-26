# Architecture
Proposed portable C++ core with no ESP-specific dependencies; host test runner, data-driven tech catalog, seeded RNG. Verify target SDK and hardware before selecting drivers.
Fixed deterministic ticks independent of rendering: environment, decisions, reservations/actions, births/deaths, knowledge, history. Stable ID iteration; bounded queues; read-only rendering snapshots. Inactive regional simulation may reduce update frequency but preserve identities and rules. Explicit embedded status/error logs, watchdog, performance metrics.
## Persistence
Two alternating checksummed checkpoints and append-only event segments. Write, sync where supported, validate readback then publish manifest. Retain last verified generation. Replay complete validated events with matching world and event IDs only; report corrupt tail. Power-cut test actual filesystem; rename alone is not a durability guarantee. Batch writes, monitor wear/free space; export archives or safe pause before data loss; never silently drop genealogy. Finite media requires archival expansion.
## Migration
Export manifest, schema/rules versions, RNG, full world, catalog and historical segments. Validate checksums, invariants and golden replay before import; keep source intact. Hardware-independent IDs and state, external backups. Original ESP32 is founding device; future hardware expands world rather than resetting it.
## Optional streaming
Publish a bounded read-only snapshot/event stream; separate rendering/broadcast host. Stream failure never alters simulation, blocks ticks or requires internet for core operation. See LIVESTREAM.md.
