# Structured diagnostics

Enable in amule.conf, then restart:

```ini
[Diagnostics]
Enabled=1
MaxMiB=8
```

The core appends UTF-8 JSON Lines to `diagnostics.jsonl` in its config directory.
Rotation retains one `.1` file; each file is bounded by MaxMiB (clamped to 1–64).
Diagnostics are disabled by default. File failures disable this sink without
changing transfer behavior. Enabled packet tracing flushes each event and adds I/O.

Each record has `schema=diag_event_v1`, `client=amule`, UTC Unix-millisecond `ts`,
a process-local increasing `seq`, `family`, `event`, `severity`, empty `keys`, and
numeric `body.code`, `body.bytes`, `body.count`. Events cover startup/shutdown,
received TCP packets (opcode and byte count), parser rejection, peer bans,
low disk space, upload slots, and active downloads. Scheduler counts are sampled
once per second. No addresses, file hashes/names, local paths, credentials, or
packet payloads are included. This envelope is inspired by eMuleBB; the limited
body schema is not a claim of full trace interchangeability.

Validated with the daemon build and StructuredDiagnosticsTest, including disabled
logging, JSON escaping/parsing, concurrent writes, and bounded rotation.
