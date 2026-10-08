# Global download buffer budget

`eMule/GlobalDownloadBufferMiB` defaults to 64 MiB. Set it to 0 to restore the
legacy per-file buffering and disable memory backpressure. The configured ceiling
is capped at 512 MiB and, where the OS reports free memory, reduced to one eighth
of free memory (with a 1 MiB minimum). Memory is sampled every five seconds.

The queue accounts for all per-file buffered bytes, including writes pending on
the background disk writer. Files can use spare budget; exhaustion flushes the
largest writable buffer and suspends socket reads until headroom returns. The
memory read quota also applies when the bandwidth setting is unlimited.

This is a buffer target, not a process RSS limit: socket queues, hash jobs, and
compressed packets expanding after a read can transiently exceed it. OS free
memory reporting may not reflect a container limit; use an explicit low ceiling
for a constrained container. A disk-full file retains its buffer and can therefore
backpressure the queue until disk-space recovery.

Validated with the daemon build and DownloadBufferPolicyTest (memory exhaustion,
refunds, recovery, unlimited bandwidth, and the bandwidth/memory intersection).
