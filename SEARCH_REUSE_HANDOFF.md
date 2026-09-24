# Search reuse fix: handoff

Recorded 2026-09-24. **Implementation is pushed; acceptance verification is not complete.**
Do not describe this as ready for review or open a replacement PR until the remaining
checks below have evidence. No replacement PR has been opened.

## Repositories and exact state

- Primary repository requested by the user: `git@github.com:3togo/amule.git`.
- Upstream source: `https://github.com/amule-org/amule.git`. Fetch/pull upstream changes,
  but push feature work to the user's repository, not upstream.
- Feature branch: `codex/reuse-running-search-v2`.
- Verified feature HEAD on both user repositories:
  `fcef14483a86d6bdd686a4e05fa047e4998a4d5f`.
- Upstream base: `18f32795fa4f913fd9821b07eb8179aeaf834106`. A fresh fetch during this
  session showed zero upstream commits missing from the feature branch. Fetch again
  when resuming; this is a snapshot, not a guarantee about future upstream changes.
- `3togo/amule` is a standalone repository (`fork: false`), so GitHub would not accept
  it as the head repository for an upstream PR. A registered fork was created:
  `git@github.com:3togo/amule-upstream.git` (parent `amule-org/amule`). The feature
  branch is already pushed there too. Keep the primary repository updated; use this
  registered fork as the eventual upstream PR bridge.
- This document is on the separate `codex/search-reuse-handoff` branch in
  `3togo/amule`, so it does not add a documentation commit to the feature PR.

### Original PR: leave its restored branch alone

Original PR: https://github.com/amule-org/amule/pull/1510 (CLOSED).
Its branch `codex/reuse-running-search` was force-pushed during earlier work, after
which GitHub closed the PR. Reopening failed even after restoring the original head.
The branch is now restored to `7e5cf7c6d682c799657302185f66bd9da877d2de`.
Do not force-push or rewrite that branch again. It still contains the unrelated
i18n commit; the new v2 branch does not.

## Changes completed

Feature commits, in order:

1. `21a72e2b0bcf339474ddb1964060ea346aca4e61`
   `feat(search): reuse an identical running search tab`
   — search-only commit cherry-picked onto upstream; unrelated i18n omitted.
2. `0a3c7824cb3b26d8946a8bdad34418e57c477794`
   `fix(search): avoid duplicate page-change notification`
   — use `ChangeSelection()` before the explicit page-change refresh.
3. `fcef14483a86d6bdd686a4e05fa047e4998a4d5f`
   `fix(search): invalidate reuse after stop and daemon restart`
   — discard stored request on explicit stop; discard request/progress when a
   restarted daemon causes result views to reset.

Only five files differ from the upstream base:

- `src/SearchRequest.h`: owned request values and exact comparison of query, search
  type, extension, file type, 64-bit size limits, and availability. Only progress
  0–100 is reusable; terminal sentinels are excluded.
- `src/SearchListCtrl.h`: optional request stored on its result page; browse pages
  excluded. Rekeying the page preserves the request; closing destroys it.
- `src/SearchDlg.cpp`: reuse before interruption prompt/backend stop; separate
  monolithic lifecycle lookup and remote reported-progress lookup; request attached
  after successful local start/optimistic remote tab creation; invalidations above.
- `unittests/tests/SearchRequestTest.cpp`: running/terminal status, every filter,
  64-bit sizes, owned values, and exact query syntax.
- `unittests/tests/CMakeLists.txt`: register the new test target.

Source audit also checked rejected-start tab removal, close cleanup, restored and
externally discovered tabs lacking a stored request, and remote ID remapping.
**Source audit is not a substitute for runtime verification.** The latest UI
invalidation calls are not directly exercised by the pure request unit tests.

## Moderator concerns and current status

Read the full review before submission:

```bash
gh api repos/amule-org/amule/pulls/1510/reviews
gh api repos/amule-org/amule/issues/1510/comments
```

- Remove unrelated i18n changes: DONE on v2. Translation fragmentation, catalogue
  regressions, installer newline changes, and their format failures are outside
  this search-only branch. They were excluded, not independently fixed here.
- Complete runtime plan in both monolithic and remote GUI: NOT DONE; checklist below.
- Windows/macOS upstream CI: NOT CONFIRMED. Fork CI is running; upstream may still
  require maintainer approval. The moderator offered approval after i18n removal.
- Optional duplicate selection callback cleanup: DONE.
- Optional redundant related-search stop: unchanged; reviewer said harmless.

Earlier claims that all concerns were resolved were premature. Preserve this
distinction in any PR description: implementation, unit tests, source audit, and
interactive acceptance are different evidence.

## Verification already performed

- Linux Release builds of `amule`, `amuled`, and `amulegui` succeeded, including
  a rebuild after the latest fixes. Use **`-j100`** for builds as the user requested.
- All **77 CTest tests passed**, including `SearchRequestTest`, after latest fixes.
- Whole-tree `src`/`unittests` clang-format 18.1.8 check passed before the final
  lifecycle edits; the changed files passed again after those edits.
- `git diff --check` against the upstream base passed.
- No native interactive acceptance checks were performed in this session.

CI snapshot: jobs running/queued, no final pass established. Inspect fresh results:

```bash
gh run list --repo 3togo/amule --branch codex/reuse-running-search-v2 --limit 10
gh run view 35987306623 --repo 3togo/amule
gh run watch 35987306623 --repo 3togo/amule --exit-status
```

Current feature-commit run IDs:

| Workflow | Run ID |
|---|---|
| C/C++ CI (Linux, Windows, macOS) | 35987306623 |
| clang-format | 35987306518 |
| clang-tidy | 35987306590 |
| Icons CI | 35987306547 |
| I18n CI | 35987306654 |

Build run: https://github.com/3togo/amule/actions/runs/35987306623
Superseded previous-commit workflows were cancelled to reduce queue contention.
The workflows build/test code, but do not complete the interactive checklist.

## Resume on another computer

Use a fresh directory, or preserve any existing local changes before switching.

```bash
git clone --branch codex/reuse-running-search-v2 git@github.com:3togo/amule.git amule-search-reuse
cd amule-search-reuse
git remote add upstream https://github.com/amule-org/amule.git
git remote add pr-fork git@github.com:3togo/amule-upstream.git
git fetch upstream master
git status --short
git log -3 --oneline
git rev-list --count HEAD..upstream/master
```

If upstream advanced, integrate it without rewriting the restored original PR
branch (for example `git merge upstream/master` on v2), resolve conflicts, and
rerun verification. Do not blindly merge into a dirty checkout.

Install platform dependencies according to the repository build instructions and
`.github/workflows/ccpp.yml`. A Linux configuration matching the tested feature set:

```bash
cmake -S . -B build-search-reuse -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON -DBUILD_MONOLITHIC=ON -DBUILD_DAEMON=ON \
  -DBUILD_REMOTEGUI=ON -DENABLE_IP2COUNTRY=NO -DENABLE_UPNP=NO
cmake --build build-search-reuse -j100
ctest --test-dir build-search-reuse --output-on-failure
git diff --check upstream/master...HEAD
```

The original computer used extracted dependencies, not system installation:

- Build: `/tmp/amule-pr1510-build-v4`.
- Dependencies: `/tmp/amule-pr1510-deps/root` (wxWidgets 3.2.11, Crypto++ 8.9,
  clang-format 18.1.8 plus LLVM dependencies).
- Runtime library path: `/tmp/amule-pr1510-deps/root/usr/lib/x86_64-linux-gnu`.
- CMake overrides there: `CMAKE_CXX_FLAGS=-I/tmp/amule-pr1510-deps/root/usr/include`,
  `CRYPTOPP_LIBRARY=/tmp/amule-pr1510-deps/root/usr/lib/x86_64-linux-gnu/libcrypto++.so.8.9.0`,
  `wxWidgets_CONFIG_EXECUTABLE=/tmp/amule-pr1510-deps/root/usr/lib/x86_64-linux-gnu/wx/config/gtk3-unicode-3.2`,
  `wxWidgets_CONFIG_OPTIONS=--prefix=/tmp/amule-pr1510-deps/root/usr`.

These `/tmp` paths are machine-local and disposable. Do not copy them into the new
machine's CMake configuration unless you have recreated that exact layout.

## Required runtime checklist — all pending

Run both modes sequentially: first `build-search-reuse/src/amule`, then quit it
and use `build-search-reuse/src/amuled` plus `build-search-reuse/src/amulegui`.
Avoid configuration locks and port conflicts. Use a separately configured test
profile (`-c /absolute/test-config-directory`) if appropriate; do not erase normal
configuration. Configure daemon EC access for the remote GUI, without recording
passwords in this document. On the original machine, prepend the runtime library
path above to launches.

Connect eD2k and bootstrap Kad. Use a legal common query such as `ubuntu`. Repeat
after more than 500 ms (the click debounce), but while the search is still running.
For remote reuse, wait for the first progress report. Record PASS/FAIL/BLOCKED and
observations for each mode; do not infer a pass from source inspection.

| Scenario | Expected observation | Modes |
|---|---|---|
| Repeat active Global search after selecting another tab | Original tab selected; no new tab, interruption prompt, backend start, or stop; progress continues | Both |
| Repeat active Kad search with another Kad query running | Original tab selected; neither search interrupted; no resend | Both |
| Change extension, type, min/max size, availability individually | New request, not reuse; eD2k confirmation appears; decline preserves old search, accept starts new request | Both |
| Stop and immediately repeat, then switch back and retry | Stopped page never absorbs new request; fresh search attempted even before remote progress catches up | Both |
| Wait for natural completion, then repeat | Fresh search rather than reuse of completed page | Both |
| Close running tab, then repeat | New tab/search; other searches unaffected | Both |
| Reject start while network disconnected, reconnect and retry | No reusable rejected tab remains; retry works | Both |
| Quit with results, restart and repeat | Restored result tabs not treated as running requests | Both |
| Peer View Files tab, then normal search | Browse page never reused as normal search | Both |
| Start searches through another client before GUI start, then repeat GUI request | Placeholder and daemon IDs differ; rekey preserves correct page/request; repeat selects that page | Remote |
| Search discovered from another client, submit same query locally | Discovered page is not silently reused; normal start/prompt/rejection path | Remote |
| Restart daemon with GUI open, reconnect and repeat | Old session's request/progress cannot reuse restored page | Remote |

For changed Kad filters, a core rejection of a concurrent same-keyword lookup is
not the same as tab reuse: verify the new-request path was actually reached.
Immediately after remote Stop, an eD2k interruption prompt may briefly persist
until the next progress report; the stopped page must still not be reused.

Unchanged tab count alone does not prove no resend. Use diagnostic logs or debugger
breakpoints on `CSearchList::StartNewSearch` / `CSearchListRem::StartNewSearch` and
stop paths to record that duplicate reuse sends no start/stop. Record actual ID
remapping evidence too. If unavailable, mark that observation BLOCKED, not PASS.
For failures record mode, filters, timing, tab counts, and relevant sanitized logs.

## Finish and submit only after verification

Fix any failures, rebuild with `-j100`, rerun tests and relevant scenarios, and
record evidence. Push feature updates without force:

```bash
git push origin codex/reuse-running-search-v2
git push pr-fork codex/reuse-running-search-v2
```

Only once the acceptance checks are complete and CI is satisfactory, prepare an
honest PR description linking #1510, documenting i18n removal and actual results.
Use the registered fork for the upstream head:

```bash
gh pr create --repo amule-org/amule --base master \
  --head 3togo:codex/reuse-running-search-v2 \
  --title 'Reuse an identical running search tab' --body-file /path/to/verified-pr-body.md
```

The body file above is a placeholder: create it with real evidence first. Check
the selected head repository in GitHub's response. Do not claim upstream CI has
passed until it has run; maintainer approval may be needed. If continuing in Codex,
attach any created PR to the task using the app's PR attachment tool.
