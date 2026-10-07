# Kad lookup diagnostics

The Kad panel's **Lookup diagnostics** button opens a snapshot of active and recently completed lookups. The local GUI reads the core directly; amulegui requests the same text with authenticated EC opcode `EC_OP_GET_KAD_LOOKUPS` (`0x69`). With an older core, the remote GUI explains that lookup diagnostics are unsupported. The request does not change the search or send Kad packets.

Each search retains up to 128 queried peers and the latest 256 events. The manager retains the last 16 completed searches and displays up to 16 active searches. Records own their values, including KadID, endpoint, times, and referral origin; they contain no pointers to contacts. History is transient and is not written to disk.

The snapshot shows routing requests and replies, latest routing round-trip time, item requests, result packets (including empty packets), received result records, and accepted referrals that move closer to the target. Result records are not unique files. An overdue request means no routing reply after three seconds; it is an observation and does not change FastKad's scheduling. Late responses can clear it.

Output is capped at 12000 Unicode characters plus a short truncation message, fitting EC's 16-bit string length even with four-byte UTF-8. When records or peers are omitted, the snapshot says so. Remote requests are on demand, not polled, and safely abort if the connection is discarded.

The behavior was informed by eMule's lookup history in reference checkout `ac3d52e`. The bounded value model, text formatter, and aMule/EC integration were written for aMule; no eMule implementation or MFC controls were transplanted.
