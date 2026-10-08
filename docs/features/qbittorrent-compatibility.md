# qBittorrent-shaped eD2k compatibility API

Enable `QBitCompatibility=1` under `[Server]` in `amuleapi.conf` and restart
amuleapi. It defaults to off. Routes live at `<BasePath>/api/v2` on the existing
HTTP listener; the native `/api/v1` API continues to work. Use username `admin`
or `guest` with the corresponding existing amuleapi password. The adapter uses
the native signed session, expiry, revocation, rate limiting and authorization,
with a `SID` cookie scoped to `/api/v2`. Guests can read but cannot mutate.
Browser requests with an Origin must match the listener or its configured CORS
allowlist. Responses containing compatibility data are private and not cached.

Supported routes:

- `auth/login`, `auth/logout`, `app/version`, `app/webapiVersion`, `app/preferences`.
- `torrents/info`, `torrents/properties`, `torrents/files`, `torrents/categories`.
- `torrents/createCategory`, `torrents/add`, `torrents/setCategory`.
- `torrents/pause` / `stop`, `resume` / `start`, `topPrio`, `delete`.
- `torrents/setShareLimits` accepts unlimited (`-1`) limits only.

Reads accept GET/HEAD and form-encoded POST; mutations use POST. Form-encoded requests and field-only
multipart requests are supported. Hashes are 32-digit eD2k hashes, separated by
`|`, with at most 100 unique hashes per mutation. `hashes=all` is supported for
mutations within that limit. Info supports hashes/category and common status
filters, sorting by name/size/progress/dlspeed, reverse, offset and limit.
Completed downloads are retained notification entries, not a second list of all
shared files. Each download has a single file. Upload speed and timestamps that
are unavailable in the download snapshot are reported as zero.

Adds accept newline-separated eD2k file links in `urls` (at most 100). Magnets,
`.torrent` uploads, arbitrary save paths and initially paused/stopped adds are
unsupported. Unknown add options are rejected rather than silently ignored. Select a category with the desired incoming path instead; pause an
accepted download with a separate request. Completed entries report `stoppedUP`
and support clearing their notification; this does not stop aMule sharing.

Cancellation removes partial data in aMule, so active downloads require
`deleteFiles=true`. Completed entries require `deleteFiles=false`, which clears
only the notification and preserves the completed file. Requests for the
opposite behavior return 409 before any mutation. Changes to completed entries,
finite seeding limits and unsupported operations fail explicitly. Batch commands
prevalidate all entries, but EC commands are not transactional: a later failure
can leave earlier operations applied. Native partial failures return 409, not a
false `Ok.`.

This is a limited compatibility surface for eD2k clients, not a BitTorrent
implementation or a claim of complete compatibility with Arr applications.
The covered subset has been exercised with qbittorrent-api 2026.10.0 at both
the root and a BasePath prefix. Arr workflows and live peer transfers still need
qualification.

With daemon, amuleapi and tests enabled, `QBitCompatTest`, `AmuleApiConfigTest`
and `QBitCompatSmoke` cover parsing, opt-in configuration and a real local
HTTP/EC workflow, both at the listener root and under a BasePath prefix. The smoke test starts disposable daemons with eD2k/Kad
disconnected, exercises both roles and transfer commands, and removes its own
profile and processes afterward.

The integration script accepts `--sdk-smoke` to additionally exercise the Python
qbittorrent-api SDK when it is installed. SDK tests cover login, version probing,
category creation, transfer reads, eD2k add, stop/start, priority, category changes
and cancellation. The optional dependency is not required by CTest.
