# Headless feedback & diagnostics forum over Kademlia notes

> **Status:** Draft (design/logic proposal)
> **Scope note:** Coding and implementation discussion is **deliberately dropped to the minimum** for the
> time being. The goal of this document is to agree on the *logic* — the forum model, admission,
> moderation, privacy, and the payload contracts — before any code is written or any PR is opened.
> Implementation details (exact classes, EC opcode numbers, file layout) will be worked out later, against
> the contracts frozen here.

## TL;DR
Add an **opt-in, headless-first, decentralized forum** that lets `amuled` users subscribe to topic boards
(e.g. an official "aMule Headless Feedback" board) and post **free-text bug/feedback** and **desensitized
per-download diagnostics** — all of it **strictly desensitized** (no IPs, filenames, or usernames) and on a
**strictly voluntary, opt-in basis**. The board is published over the **existing** Kademlia notes primitive —
**no new wire-protocol opcodes**. Each forum is administered by an admin who controls **admission via signed
tickets** and **moderation via signed directives** (ban by author ID or IP, revoke, pin/lock), and a user is
never disturbed unless they have **subscribed**. The point is to collect field data we currently lack — so we
can improve the headless protocol and, where the data shows it's needed, fine-tune Kademlia itself — in
particular to capture exactly what happens when Kad downloads are abnormal, failing, or slow.

## Motivation
The headless stack (`amuled` + EC + `amuleweb`/`amulecmd`/`webapi`) is how many users run aMule, yet it is
the hardest path from which to get bug reports: there is no "report a problem" affordance, and users rarely
file detailed GitHub issues. More fundamentally, **we have no field data at all** about how Kademlia behaves
in the wild — above all, *when Kad downloads are abnormal, failing, or slow* — so we cannot tell when, or how,
to fine-tune Kad. A one-command, **strictly voluntary and opt-in**, privacy-preserving, **subscribable** board
from the daemon itself gives maintainers structured, reproducible signal — **every submission desensitized**
(no IPs, filenames, or usernames) — of bug reports and per-download diagnostics captured precisely when a Kad
download misbehaves: source/connection aggregates, gap lists, corruption stats, and timing/speed traces. That
is the exact data needed to decide *whether and how* to tune Kad, gathered without invasive telemetry and only
because users *chose* to share it.

## Goals
- A real **forum** abstraction: subscribable boards with an admin who controls admission and moderation.
- Submit bug/feedback and desensitized download diagnostics **from the headless client** (`amulecmd` /
  `webapi` / EC), with zero GUI dependency (core-side code).
- Reuse the **existing** Kad notes infrastructure — no new Kademlia packet types, no interop risk.
- Strong privacy: default-deny field allowlist, no IPs/filenames/usernames in post payloads; author
  identity is a local pseudonym; explicit preview-before-submit; **disabled / not joined unless the user
  subscribes**.
- Admin authority over admission ("accept all but" or "reject all but") and over moderation (ban by author
  ID or IP, revoke, pin/lock, policy changes).
- Capture diagnostics **automatically when a Kad download is abnormal, failing, or slow** (not only on
  demand), so the field data arrives exactly at the moments that justify tuning Kad.
- **Strictly voluntary and desensitized by construction:** no data leaves the daemon unless the user has
  subscribed to a forum *and* explicitly submits; every payload is desensitized via the default-deny allowlist
  and is previewed before submit. Nothing is collected silently or by default.

## Non-goals (v1)
- No new Kad opcodes (gossip/relay is a deferred, opt-in extension).
- No server/collector backend; the board is pure Kad. A *voluntary* admin/subscriber node gathers by
  subscribing — no special infrastructure required.
- No coding-level detail in this document (see Scope note).

## Forum model
A **forum** is a self-describing, signed object:

```
ForumDescriptor = {
    forum_id,            // stable identifier
    display_name,
    keyword_shards[],    // one or more Kad keyword hashes (sharding for capacity)
    admin_pubkey,        // admin authority
    policy,              // ADMIT_ALL_BUT | REJECT_ALL_BUT
    allow_diag, allow_bug, ...   // per-forum content policy (optional)
    signature            // by admin_pubkey
}
```

- **Subscription (opt-in, no disturbance):** a user is not disturbed and generates **no** Kad traffic for a
  forum they have not subscribed to. Subscribing locally stores the descriptor and only then enables
  periodic `SEARCH_NOTES_REQ` + publish for those keyword shards. Because the board is **pull-based**
  (you search on a timer; nothing is pushed), an unsubscribed user is never contacted or spammed.
- **Admin rights:** the admin's authority is the `admin_pubkey` in the descriptor. All admission and
  moderation actions are **signed directives** that subscribing clients verify and enforce. The admin has
  the full set of rights needed to run the forum.

## Admission: ticket-gated (the core logic)
To be **visible to others**, a post must carry or reference a **valid, unrevoked admin-signed ticket**
binding its `author_id`. The protocol therefore defaults to **deny-at-display**; the *admission scheme* is
purely a matter of **how the admin issues tickets**:

- **REJECT_ALL_BUT** ("deny by default"): the admin issues a ticket **only** to explicitly approved
  authors (manual allowlist). Everyone else is invisible by construction. Strongest moderation; the user
  must be approved before posting.
- **ADMIT_ALL_BUT** ("allow by default"): the admin grants openly — a **wildcard/standing ticket** or an
  auto-issuing admin bot watching a request keyword — and removes bad actors afterwards via
  `ban`/`revoke`. Default-allow, reactive cleanup, but still routed through the same ticket primitive.

Both schemes share **one client code path**; only the admin's issuance policy differs. This also makes
moderation far stronger than after-the-fact deletes: in REJECT_ALL_BUT a post cannot appear without an
admin signature.

**Ticket** = admin-signed `{ forum_id, author_id, issued_at, expires?, nonce }`, published as a note under
the forum's keyword(s). **Revocation** = signed `revoke(ticket_id | author_id)`.

**How a user obtains a ticket (headless-friendly):** for REJECT_ALL_BUT the user publishes a signed
*ticket-request* note (their `author_id`); the admin (or an admin bot) issues a ticket note back. For
ADMIT_ALL_BUT the wildcard ticket means no request step. All in-band over Kad notes — no website/DM needed,
so it works from `amuled`.

## Moderation directives (signed by admin)
```
ban_author(author_id)     // primary; author_id is in the post payload, enforced by all subscribers
ban_ip(ip_subnet)         // store-time enforcement; see below
revoke(ticket_id|author_id)
pin / lock(thread)
set_policy(...)           // e.g. switch ADMIT_ALL_BUT <-> REJECT_ALL_BUT, require author identity
add_coadmin(pubkey)
```
- **Ban by author ID** is the clean, pull-friendly, privacy-preserving axis: the ID is a self-generated
  pseudonymous key carried and signed in the note; subscribers drop banned IDs.
- **Ban by IP** has a known caveat in a pull model: when you *search*, the response comes from the
  responsible/responding node, not the original publisher, so a searcher never sees the poster's IP.
  IP bans are therefore enforced at **publish-receipt (store) time** by subscribed enforcing nodes, using
  the publish packet's source IP. aMule **already tracks publishing IPs per /24 subnet**
  (`CKeyEntry::sPublishingIP` / `AdjustGlobalPublishTracking`, `src/kademlia/kademlia/Entry.h`), which we
  reuse as the enforcement point. Honest limitation: soft and only on nodes running our client and
  subscribed; an abuser can rotate IPs, but ID-ban + revoke remain as backstops.

## Privacy & desensitization
- **Default-deny field allowlist:** anything not explicitly listed is never serialized into a post.
- **Never** included: IP addresses, usernames, user-hashes, filenames, or paths.
- **Author identity** is a locally generated pseudonymous keypair (not PII); the user may reset it.
- **Tickets and bans** reference the pseudonym only — no IP/real identity in the public payload. Ordinary
  readers never see IPs; only the admin's signed directives and enforcing nodes' local store-time checks
  involve IPs.
- Optional geo-context: **country code only** (via IP2Country) as an aggregate.
- Optional, **off by default**: the ED2K file hash (content-identifying).
- The client shows the exact payload **before** submitting and requires confirmation.

### Desensitized diagnostics bundle (allowlisted fields)
Built from `CPartFile` / `CDownloadQueue` / `CCorruptionBlackBox` (the last stores raw sender IPs and the
filename — both **dropped**):
- progress: bytes_done/total, parts_complete/total, **gap list** (chunk indices only)
- source aggregates (counts only, no IPs/names): total, a4af, connected, queued
  (`GetSourceCount()` / `GetSrcA4AFCount()` at `src/PartFile.h:194`)
- client-version histogram (aggregate, no per-client identity)
- network state: kad_connected, server_connected, last_download_error, status
- timing/speed trace (aggregate): avg/inst speed, time-to-first-source, time-to-first-data, stall duration
- corruption aggregates from `CCorruptionBlackBox`: good/bad byte totals + per-part flags only (IP-keyed
  maps and `m_fileName` dropped)
- **capture triggers (the valuable part):** auto-snapshot when a Kad download is **abnormal / failed / slow** —
  e.g. speed below threshold for N minutes, status stuck, source count high but gap not closing, or a Kad
  lookup/connect error. Triggered captures (not on-demand dumps) are the data that tells us *which* Kad
  behavior to tune.

## Architecture
```
 submitter (amuled / amulegui)
   │  CFeedbackManager (core, GUI-independent)
   │  build payload (default-deny allowlist) + author_sig + ticket_ref
   ▼
 KADEMLIA2_PUBLISH_NOTES_REQ ──► Kad network (responsible nodes store note)
   │                              ≤150 notes/keyword, 5h TTL, sharded for capacity
   ▼
 subscribers (users, admin/gatherer)
   SEARCH_NOTES_REQ → verify ticket + admin directives → display
 admin: issues tickets + moderation directives (signed)
```
Control channels (all core-side, automatically headless): EC opcodes (shared by `amulegui`, `amulecmd`,
`amuleweb`/`webapi`); `amulecmd` commands; `webapi` endpoints with a preview step.

## Network-protocol impact (the important part for review)
This proposal **adds no new Kademlia opcodes**. It reuses:
- `KADEMLIA2_PUBLISH_NOTES_REQ` / `KADEMLIA2_PUBLISH_KEY_REQ` (`src/kademlia/net/KademliaUDPListener.cpp`)
- `KADEMLIA2_SEARCH_NOTES_REQ` / `KADEMLIA_SEARCH_NOTES_RES` (publish/search path in
  `src/kademlia/kademlia/Search.cpp`)

The payload is a normal note (a small tag blob) published against a **fixed, well-known keyword** (e.g.
`"amule-headless-feedback"`), hashed to a `CUInt128` exactly like ordinary Kad keyword publishes.
Unmodified eMule/aMule clients already store and relay such notes; only our client interprets the payload.
The only Kad-layer change is *calling the existing publish/search functions with our keyword*.

### Capacity & lifetime constraints (verified)
From `src/include/protocol/kad/Constants.h`:
- `KADEMLIAMAXNOTESPERFILE = 150` — at most **150 notes per keyword**; oldest are evicted
  (`src/kademlia/kademlia/Indexed.cpp:798`).
- `KADEMLIAREPUBLISHTIMES = 5h` — a published note **expires after 5 hours** unless re-published
  (`src/kademlia/kademlia/Entry.cpp`).
- `SEARCHSTORENOTES_LIFETIME = 100s` is the publish *operation* timeout, not entry lifetime.

Consequences: capacity is ~150/keyword (× Kad replication); we **shard by keyword** to multiply capacity;
the admin/gatherer must pull well within 5h and may re-seed what it collected so reports outlive a
submitter going offline. Notes are small (tag blobs); bundles stay small or are chunked across several
notes carrying `bundle_id`/`index`/`total`.

## Frozen contracts (to be agreed here; coding later)
1. Forum descriptor schema + signature (above).
2. Ticket schema + issuance/verification rule (above).
3. Post-note schema: `{ schema_version, type(bug|diag|text), forum_id, author_id, author_sig,
   ticket_ref, content, content_hash }`.
4. Moderation-directive schema (ban_author, ban_ip, revoke, pin, lock, set_policy, add_coadmin).
5. Diagnostics allowlist (§Privacy).
6. Fixed keyword constant(s) + hashing rule + sharding scheme.

## Delivery plan (high-level; coding detail deferred)
- **PR0 (docs):** this proposal.
- **PR1:** forum descriptor + subscription store (local config) + no-op control stubs.
- **PR2:** Kad publish/search wired to our keyword(s); sharding; gatherer/subscriber search loop.
- **PR3:** ticket issuance + verification (display gate) implementing both admission schemes.
- **PR4:** desensitizer + `bug`/`diag`/`text` payloads + moderation enforcement (ban by ID/IP, revoke).
- **PR5:** `amulecmd` + `webapi` submit/list/preview; GUI context menu (optional).
- **PR6 (optional, later):** gossip opcode for larger bundles (opt-in, namespaced extension).

Each PR fills `.github/pull_request_template.md` (`## Summary` + `## Test plan`) and references this
discussion.

## Risks & mitigations
- **Capacity 150/keyword, 5h TTL** → shard keywords; admin/gatherer pulls <5h and re-seeds.
- **Soft moderation** (decentralized, client-enforced) → ticket-gated admission + signed directives;
  REJECT_ALL_BUT gives pre-publish screening; ID/revoke backstops for IP rotation.
- **Spam/abuse** → ticket admission + Kad publish-tracking trust (per-/24) + content-hash dedup + local
  ignore list.
- **Privacy** → default-deny allowlist + pseudonymous author ID + preview; no PII by construction.
- **Network load** → rides existing publish cadence; participation only when subscribed; low frequency.

## Testing (high-level)
Extend `unittests/curl-tests/amuleapi/` (mirror `18-categories-crud.sh`): subscribe, submit a `bug` and a
`diag` via `amulecmd`/`webapi`, assert the note is published and re-found by a second `amuled` searching
the keyword; assert the `diag` payload contains **no** IP/filename fields; assert a banned `author_id` is
filtered and an un-ticketed post is invisible under REJECT_ALL_BUT.

## Backward compatibility
None broken — no new opcodes, no required GUI changes, feature inert until a forum is subscribed. Works on
`amuled` and the GUI app alike (core-side).

## Open questions for reviewers
- Keyword sharding scheme (category vs time-bucket)?
- JSON-in-note vs EC-tag encoding for payloads?
- Should the project run an official voluntary gatherer / admin bot?
- Ticket-request flow: pure in-band, or with an optional out-of-band gateway?

## References
- `src/kademlia/net/KademliaUDPListener.cpp` (notes opcodes)
- `src/kademlia/kademlia/Search.cpp` (publish/search notes path)
- `src/include/protocol/kad/Constants.h` (`KADEMLIAMAXNOTESPERFILE=150`, `KADEMLIAREPUBLISHTIMES=5h`)
- `src/kademlia/kademlia/Indexed.cpp:798` (eviction)
- `src/kademlia/kademlia/Entry.h` (`sPublishingIP` / `AdjustGlobalPublishTracking`, per-/24 trust)
- `src/CorruptionBlackBox.h`, `src/PartFile.h:194`
- `docs/EC_Protocol.md`, `.github/pull_request_template.md`
