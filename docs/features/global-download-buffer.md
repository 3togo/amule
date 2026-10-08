# Global download buffer budget

`eMule/GlobalDownloadBufferMiB` defaults to 64 MiB. Set it to 0 to restore the
legacy per-file buffering and disable memory backpressure. The configured ceiling
is capped at 512 MiB and, where the OS reports free memory, reduced to one eighth
of free memory (with a 1 MiB minimum). Memory is sampled every five seconds.

The queue accounts for all per-file buffered bytes, including writes pending on
the background disk writer. Files retain their existing per-file limit; exhaustion flushes the
largest writable buffer and suspends file-data reads until headroom returns. The
memory read quota also applies when the bandwidth setting is unlimited.

This is a buffer target, not a process RSS limit: socket queues, hash jobs, and
compressed packets expanding after a read can transiently exceed it. OS free
memory reporting may not reflect a container limit; use an explicit low ceiling
for a constrained container. A disk-full file retains its buffer and can therefore
backpressure the queue until disk-space recovery.

Validated with the daemon build and DownloadBufferPolicyTest (memory exhaustion,
refunds, recovery, unlimited bandwidth, and the bandwidth/memory intersection).

The existing per-file buffer limit remains an upper bound. Memory headroom
applies to file-data payload reads; peer control messages and packet headers
still obey the download bandwidth cap but do not spend file-buffer headroom.

A control packet queued behind file data on the same TCP stream must still wait
for that data; the quota cannot bypass TCP ordering.
