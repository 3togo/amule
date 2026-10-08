# Broadband upload diagnostics

When the Local Client debug category is enabled in a Debug build, emit aggregate upload diagnostics at most once every ten seconds. Include configured capacity (zero means unlimited or unknown), measured network rate, active/waiting counts, underfill age, queued logical payload, requested blocks, pending disk reads, idle peers and peers with queued output. Counts describe observations, not confirmed causes of a bottleneck. Busy snapshots are reported separately and defer collection instead of waiting for disk reads. No peer addresses, identities or filenames enter these records. Snapshot collection is skipped while the category is disabled.

## Validation

`UploadDiagnosticsTest` exercises policy boundaries. Build the daemon independently and combine all five changes to check interaction.

Before upstream submission, compare baseline and changed builds on controlled upload workloads: slow and fast peers, low and high RTT, limited peer supply, slow disk reads, session churn, and finite versus unlimited capacity. Record useful payload throughput, wire overhead, active slots, queue waiting time, memory use and disk latency. Check that extra slots and retention improve useful throughput without starving waiting peers. Windows and macOS builds and live network measurements remain required review evidence; policy tests alone do not demonstrate a broadband speedup.
