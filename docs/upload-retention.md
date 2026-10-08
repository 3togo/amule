# Productive upload sessions

After the existing 10 MiB or one-hour session limit, a productive ordinary session may continue while finite broadband capacity has been continuously underfilled. Productivity requires at least half the configured per-slot rate, with a 3 KiB/s minimum. The extension is bounded to two minutes from its first grant; fluctuating conditions cannot restart it. Friend and release-slot rules retain their existing precedence. Normal rotation resumes when utilization or productivity ceases to qualify.

## Validation

`UploadRetentionTest` exercises policy boundaries. Build the daemon independently and combine all five changes to check interaction.

Before upstream submission, compare baseline and changed builds on controlled upload workloads: slow and fast peers, low and high RTT, limited peer supply, slow disk reads, session churn, and finite versus unlimited capacity. Record useful payload throughput, wire overhead, active slots, queue waiting time, memory use and disk latency. Check that extra slots and retention improve useful throughput without starving waiting peers. Windows and macOS builds and live network measurements remain required review evidence; policy tests alone do not demonstrate a broadband speedup.
