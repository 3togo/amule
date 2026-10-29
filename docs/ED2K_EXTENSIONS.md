# OFFERFILES v1: server-paced file publication

OFFERFILES v1 lets an eD2k server tell the client how fast it may publish its
shared files. aMule asks for it at login and follows the pace the server
advertises. A server that advertises nothing, Lugdunum included, gets the
standard pace: at most 200 files per packet, one packet a minute.

The setting is on by default: **Preferences → Server → Publish shared files
faster on servers that allow it**. amulegui, amuleapi
(`servers.accelerated_publication_enabled`) and the Web UI can set it on a
daemon, or set `ED2KAcceleratedPublication=0` in the `[eMule]` section of
`amule.conf`. A change applies from the next server connection.

## Wire format

**Request.** `OP_LOGINREQUEST` carries one more tag after the four standard
ones: the string-named uint32 tag `offerfiles_v = 1` (type `0x03`, uint16 name
length, case-sensitive name, uint32 value). A server that does not know it
ignores it.

**Advertisement.** A v1 server answers only a client that asked, in its
post-login `OP_SERVERIDENT`. Each field appears exactly once:

| Field | Encoding | Valid when |
|---|---|---|
| `offerfiles_v` | string-named uint32 | exactly 1 |
| `offerfiles_batch_max` | string-named uint32 | greater than 0 |
| `offerfiles_min_interval_ms` | string-named uint32 | greater than 0 |
| `ST_SOFTFILES` | numeric uint32 | greater than 0 |
| `ST_HARDFILES` | numeric uint32 | greater than `offerfiles_batch_max` |

Only uint32 encodings are valid. A missing, duplicate, wrong-type or
inconsistent field invalidates the whole advertisement, and the connection
stays at the standard pace.

The soft limit is the number of distinct files the connection may offer. The
hard limit is a per-packet boundary: every packet stays strictly below it. The
server skips a v1 packet larger than `offerfiles_batch_max`, without
disconnecting.

## Client behaviour

- **Snapshot.** The first valid advertisement binds the connection until it
  closes. A later SERVERIDENT is ignored and cannot relax it. A malformed one
  does not disconnect.
- **Waiting for it.** aMule holds its first offer until the SERVERIDENT
  arrives, for at most 5 seconds after the ID assignment. ed2k-server sends it
  in the same login batch as the ID; Lugdunum sends it only in answer to
  `OP_GETSERVERLIST`. When the wait runs out, aMule publishes at the standard
  pace.
- **A late advertisement** still binds the connection. If a packet sent before
  it was larger than `offerfiles_batch_max`, the server skipped it, so aMule
  offers the files of this connection again within the new limits.
- **Packet size.** At most the advertised batch, hard limit minus one, and 200.
  Only files not yet offered on the connection use the soft budget; a re-offer
  after a rename or completion fills a packet slot but no budget.
- **Pace.** At least the advertised interval and 500 ms, so at most 400 files a
  second, with no catch-up bursts. The interval runs from the moment the socket
  accepts the packet.
- **Live limits.** Soft and hard limits from later UDP status replies can
  tighten the snapshot, never widen it. Nothing negotiated is saved in
  `server.met`.
- **Turning it off** during a session keeps the session's limits and slows to
  the standard one-minute pace until the next connection.

aMule has no acknowledgement of indexing: it counts files offered, not files
the server indexed. A re-offer refreshes the source's complete or partial
state; ed2k-server keeps the file name from the first indexing.

## Tests

`OfferFilesPolicyTest` covers the policy. `OfferFilesLiveLimitsTest` and
`OfferFilesInteropTest` run a real amuled; the interop test also needs an
ed2k-server binary and runs only against loopback. See the docstring of
`unittests/tests/OfferFilesInteropTest.py`.
