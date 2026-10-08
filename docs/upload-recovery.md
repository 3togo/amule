# Stalled upload recovery

With sustained broadband underfill and an eligible waiting replacement, recycle an ordinary slot after 30 seconds with no requested or queued work, or 60 seconds without progress when work is present. Positive observed throughput resets stall grace. Pending disk reads, timer gaps, clock regression, and loss of eligibility reset grace. Friend and release slots are excluded. Recovery shares the one-client-per-cycle kicking constraint, sends the existing out-of-part-requests notification, and requeues the peer with a 60-second retry cooldown. Busy disk snapshots also reset grace. Main-loop counter updates and snapshots defer on busy block-list locks instead of waiting for synchronous reads. Disk requests retain a shared atomic counter independent of client lifetime; worker threads do not manipulate main-thread client references.

## Validation

`UploadRecoveryPolicyTest` exercises policy boundaries. Build the daemon independently and combine all five changes to check interaction.

Before upstream submission, compare baseline and changed builds on controlled upload workloads: slow and fast peers, low and high RTT, limited peer supply, slow disk reads, session churn, and finite versus unlimited capacity. Record useful payload throughput, wire overhead, active slots, queue waiting time, memory use and disk latency. Check that extra slots and retention improve useful throughput without starving waiting peers. Windows and macOS builds and live network measurements remain required review evidence; policy tests alone do not demonstrate a broadband speedup.
