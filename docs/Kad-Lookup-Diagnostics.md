# Kad lookup diagnostics

The Kad panel's **Lookup diagnostics** button opens a snapshot of active and recently completed lookups. The local GUI reads the core directly; amulegui requests the same text with authenticated EC opcode `EC_OP_GET_KAD_LOOKUPS` (`0x69`). With an older core, the remote GUI explains that lookup diagnostics are unsupported. The request does not change the search or send Kad packets.

Each search retains up to 128 queried peers and the latest 256 events. The manager retains the last 16 completed searches and displays up to 16 active searches. Records own their values, including KadID, endpoint, times, and referral origin; they contain no pointers to contacts. History is transient and is not written to disk.

The snapshot shows routing requests and replies, latest routing round-trip time, item requests, result packets (including empty packets), received result records, and accepted referrals that move closer to the target. Result records count only endpoints sent an item request and are not unique files. An overdue request means no routing reply after three seconds; it is an observation and does not change FastKad's scheduling. Late responses can clear it.

Output is capped at 12000 Unicode characters plus a short truncation message, fitting EC's 16-bit string length even with four-byte UTF-8. When records or peers are omitted, the snapshot says so. Remote requests are on demand, not polled. A single modeless snapshot window leaves connection controls accessible. Repeated requests refresh that window. The button is disabled while a remote request is pending; a reply or connection loss restores it. Connection loss replaces stale text with a retry message. Replies arriving after the window or its parent closes safely discard their output.

The behavior was inspired by eMule's lookup history in reference checkout `ac3d52e`. The bounded value model, text formatter, and aMule/EC integration were written for aMule; no eMule implementation or MFC controls were transplanted.

Native GUI lifecycle tests cover valid and malformed responses, older cores, connection loss and retry, and replies after window or parent destruction. A daemon integration test uses a UDP endpoint bound to an address owned by the test machine to exercise production search tracing, stop/archive, and EC reconnect. It skips when no suitable local address is available; the empty-state EC test remains unconditional.
