# Adaptive upload read-ahead

Slow slots retain a one-block target; fast slots target two seconds of observed throughput, with a ten-block floor and a 32-block ceiling. Targets also share a 64 MiB outstanding logical payload budget across active slots. Each protocol request must fit the remaining global budget before its read starts. A request may exceed the nominal per-slot target because protocol requests are indivisible. Existing backlog is retained if above the budget; further reads wait for it to drain. The first serviced slot rotates between passes to avoid fixed-order starvation. This is a payload accounting budget, not an RSS limit: packet metadata and transient compression/read copies consume additional memory.

## Validation

`UploadReadAheadPolicyTest` exercises policy boundaries. Build the daemon independently and combine all five changes to check interaction.

Before upstream submission, compare baseline and changed builds on controlled upload workloads: slow and fast peers, low and high RTT, limited peer supply, slow disk reads, session churn, and finite versus unlimited capacity. Record useful payload throughput, wire overhead, active slots, queue waiting time, memory use and disk latency. Check that extra slots and retention improve useful throughput without starving waiting peers. Windows and macOS builds and live network measurements remain required review evidence; policy tests alone do not demonstrate a broadband speedup.
