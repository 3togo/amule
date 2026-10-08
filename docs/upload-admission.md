# Adaptive upload admission

Finite configured upload budgets of at least 512 KiB/s are considered underfilled below 95% utilization. After ten seconds of continuous underfill, allow up to 25% extra slots (at least one), bounded by the existing 250-client ceiling. Existing one-second admission pacing and socket limits remain in force. Unlimited capacity cannot be inferred and does not activate elasticity. A timer gap over five seconds, clock regression, or capacity change resets warmup. Extra slots are retired through normal session fairness, rather than abruptly dropping productive peers.

## Validation

`UploadAdmissionTest` exercises policy boundaries. Build the daemon independently and combine all five changes to check interaction.

Before upstream submission, compare baseline and changed builds on controlled upload workloads: slow and fast peers, low and high RTT, limited peer supply, slow disk reads, session churn, and finite versus unlimited capacity. Record useful payload throughput, wire overhead, active slots, queue waiting time, memory use and disk latency. Check that extra slots and retention improve useful throughput without starving waiting peers. Windows and macOS builds and live network measurements remain required review evidence; policy tests alone do not demonstrate a broadband speedup.
