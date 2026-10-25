> Current recurring-CI runbook:
> [.github/workflows/CI_TROUBLESHOOTING.md](.github/workflows/CI_TROUBLESHOOTING.md).
> It records the runtime-install timeout and `nullptr` fixes, how to detect
> their reintroduction, and the checks to repeat. Older sections below contain
> historical tool versions; consult the current workflows before reusing commands.

# Lesson Learnt — PR #1713 (Kad AICH bounded selection) build error

> **Correction to the record.** The PR conversation mentions two issues (a
> `std::map` extracted-node key reassignment and a `clang-format` 18 failure).
> Those were **already fixed in HEAD** (`0797ecb` "make bounded-eviction
> portable" and `dba771b` "format safe mismatched-key merge") before the CI run
> that actually broke. The build error that failed **all 8 build jobs + both
> clang-tidy jobs** on the PR was a *third, different* problem — see §1. Always
> read the CI logs; the discussion thread is not the source of truth.

## 1. Actual CI break: incomplete type in a test stub (the real failure)

CI run `36967078978` failed every `Build CMake *` job and both `clang-tidy`
jobs. The `Check clang-format` job **passed**. The compile error was:

```
unittests/tests/AICHSearchResultStubs.cpp:148:23:   required from here
/usr/include/c++/13/bits/new_allocator.h:165:13:
  error: invalid application of '__alignof__' to incomplete type 'CUPnPPortMapping'
... (also: invalid use of incomplete type, static_assert value type is destructible)
```

Trace: `AICHSearchResultStubs.cpp:148` defines `CamuleApp::~CamuleApp()` →
implicitly destroys the member `std::vector<CUPnPPortMapping> m_upnpMappings`
→ the `vector` destructor needs a **complete** `CUPnPPortMapping`.

**Root cause:** `src/amule.h:453` declares (under `ENABLE_UPNP`)
`std::vector<CUPnPPortMapping> m_upnpMappings;` inside `CamuleApp`. Production
`src/amule.cpp` includes `UPnPBase.h` (line 121) — where `CUPnPPortMapping` is
fully defined — before defining `~CamuleApp()`. The new test stub
`AICHSearchResultStubs.cpp` *also* defines `~CamuleApp()` but never included
`UPnPBase.h`, so the member's `vector` destructor was instantiated on an
incomplete type. `ENABLE_UPNP=YES` in CI is what exposes the member.

**Fix (verified, pushed as `7bcef57`):**

```cpp
#ifdef ENABLE_UPNP
#include "UPnPBase.h" // Needed for CUPnPPortMapping (CamuleApp::m_upnpMappings)
#endif
```

The include is `#ifdef ENABLE_UPNP`-guarded to mirror `amule.h`, and the
`AICHSearchResultTest` target already has `${CMAKE_SOURCE_DIR}/src` on its
include path plus `config.h` (defining `ENABLE_UPNP`) in `${CMAKE_BINARY_DIR}`.

**Second layer — the test target also needs libupnp's include path.** Adding the
`#include "UPnPBase.h"` above is necessary but *not sufficient*: `UPnPBase.h`
includes `UPnPCompatibility.h`, which does `#include <upnp.h>` (a libupnp header
outside the repo). Production targets get that path because `muleappcommon`
links `UPNP::Shared` (`src/CMakeLists.txt:830-833`), but `AICHSearchResultTest`
links only `mulecommon`, so it never inherits it — and the build fails with
`fatal error: 'upnp.h' file not found` (seen on Ubuntu **and** macOS). Fix: in
`unittests/tests/CMakeLists.txt`, add the imported target's include dirs to the
test target, mirroring the `muleappcore` pattern (`src/CMakeLists.txt:923-926`):

```cmake
if (ENABLE_UPNP)
    target_include_directories (AICHSearchResultTest PRIVATE
        $<TARGET_PROPERTY:UPNP::Shared,INTERFACE_INCLUDE_DIRECTORIES>)
endif()
```

`UPNP::Shared` is an imported target created at top-level scope by `cmake/upnp.cmake`
(so it is visible in the `unittests/` subdirectory), and its
`INTERFACE_INCLUDE_DIRECTORIES` already carries the correct libupnp path on every
platform (Ubuntu pkg-config, macOS Homebrew, MinGW).

**Rule of thumb — destructors need complete member types.** Defining *any*
destructor (or instantiating one) in a translation unit requires every
non-static data member's type to be complete, because member destructors are
implicitly invoked. `std::vector<T>` destruction in particular requires `T`
complete at the point of instantiation. When you stub out an application class
in a test and provide out-of-line destructor / virtual-anchor definitions,
include **every** header that fully defines the types of that class's data
members — and guard the include with the **same feature macro** (`ENABLE_UPNP`)
the original class uses.

## 2. `std::map` extracted-node key is `const` (portability trap) — already fixed in HEAD

Retained as a general portability lesson; this was **not** the cause of the CI
run above (it was already corrected by `0797ecb` before that run).

`std::map::node_type::key()` returns `const key_type&`. Reassigning the key of
an extracted node is **ill-formed**:

```cpp
auto n = m_entries.extract(worst);
n.key() = subnet;            // ill-formed: key() is const key_type&
m_entries.insert(std::move(n));
```

- **libstdc++ (g++) is lenient** and lets this slide; local `g++` build passes.
- **libc++ (Clang) is strict** → build breaks on Clang CI.

**Fix pattern:** never reassign an extracted node's key. Detach the worst node
and insert the newcomer under its own key, restoring the detached node only on
insertion failure so the retained-set size / monotone cutoff still hold.

**Rule of thumb:** a clean `g++` build is NOT proof of standard-conformance.
Test against libc++ (`clang++ -stdlib=libc++ -std=c++17`) before pushing when
you touch node-handle / extract / try_emplace code.

## 3. Local toolchain must match CI (source of truth)

| Concern        | CI setting (source of truth)                                              | What to use locally                        |
|----------------|---------------------------------------------------------------------------|--------------------------------------------|
| clang-format   | `.github/workflows/clang-format.yml`: `clang-format-version: '18'` (jidicula action v4.18.0, `ubuntu-24.04`), checks `src/` + `unittests/` | **clang-format 18** (`clang-format --version`) |
| C++ standard   | `CMakeLists.txt`: `CMAKE_CXX_STANDARD 17` + `CMAKE_CXX_STANDARD_REQUIRED ON` | **C++17** (`-std=c++17`)                  |
| C standard     | not pinned in CMake; libutp public interface targets C11                   | default compiler C std; C11 for libutp ABI |
| feature flags  | CMake passes `-DENABLE_UPNP=YES` (and other `-DENABLE_*` flags)            | build with the **same** `-DENABLE_*` flags the CI job uses |
| clang-tidy     | `.github/workflows/clang-tidy.yml`: **LLVM/clang-tidy 18**. Tier-1 (whole tree, bug checks, `.clang-tidy`) + Tier-2 (changed lines only, maintainability checks, `.clang-tidy-new-code`, scans `src/**` only) | **clang-tidy 18** (`clang-tidy-18 --version`) |

> Note: the "C17" often mentioned in conversation means **C++17** here (the CMake
> pin is `CMAKE_CXX_STANDARD 17`). The project does not pin a plain-C standard.
>
> Crucially, feature macros like `ENABLE_UPNP` change which data members a class
> *has*. A class compiled with `ENABLE_UPNP=YES` carries `m_upnpMappings`; the
> same class compiled without it does not. Stub / test TUs must match the CI's
> flag set, or they will see a different (incomplete) class layout.
>
> clang-tidy Tier-2 uses `modernize-*` / maintainability checks from
> `.clang-tidy-new-code` and only inspects **changed `src/**` lines** (it excludes
> `unittests/`), so `NULL` in a test file is not caught there — but `time(NULL)`
> on a changed line in `src/` is. These are *separate* from the compiler warnings.

### Verify locally before pushing
```bash
# clang-format (must be v18) — fail if any file differs
clang-format --version                       # expect 18
git diff --name-only HEAD | grep -E '^src/|^unittests/' | \
  xargs clang-format --dry-run --Werror

# libc++ conformance smoke test for node-handle code
clang++ -stdlib=libc++ -std=c++17 -fsyntax-only path/to/file.cpp

# Reproduce an ENABLE_UPNP=YES build to catch incomplete-type stub errors
cmake -S . -B build -DENABLE_UPNP=YES -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target AICHSearchResultTest

# clang-tidy 18 — Tier-2 maintainability checks on changed src/ lines.
# Needs a build dir with compile_commands.json (the cmake step above creates it).
clang-tidy-18 --version                      # expect 18
git diff -U0 HEAD~1...HEAD -- 'src/**' \
  | clang-tidy-diff-18.py -clang-tidy-binary clang-tidy-18 \
        -p1 -path build -config-file .clang-tidy-new-code -quiet
# (-config-file .clang-tidy for the Tier-1 bug-oriented checks; replace HEAD~1
#  with the PR base SHA to mirror CI's diff base exactly.)
```

> Tip: the `clang-tidy-diff-18.py` wrapper and the `.clang-tidy` / `.clang-tidy-new-code`
> configs ship in the repo, so the local command above closely reproduces the CI
> Tier-2 gate. Run it before pushing to catch `modernize-use-nullptr` (e.g.
> `time(NULL)` → `time(nullptr)`) and friends on your changed lines.

## 4. `clang-format` style is non-default
`.clang-format` is `BasedOnStyle: LLVM` but overrides many knobs
(`IndentWidth: 8`, `UseTab: ForContinuationAndIndentation`, `ColumnLimit: 110`,
`PointerAlignment: Right`, `BinPackArguments/Parameters: false`, custom brace
wrapping). Run the project's `clang-format` (v18) — do not trust an IDE's bundled
or newer/older formatter; even a one-line change can be reformatted differently.

## 5. Don't trust a green local `g++` build for CI
CI builds with Clang + libc++ and feature flags (`-DENABLE_UPNP=YES` etc.) that
differ from a default local `g++` setup. libstdc++'s leniency masks ill-formed
code, and a different flag set changes which class members exist (see §1). When
in doubt, build against Clang + libc++ with the CI's feature flags locally, or
wait for the `ccpp.yml` / `clang-format.yml` CI jobs.

## 6. Verify against the CI log, not the PR discussion
The PR thread's "two fixes" were already in HEAD when CI failed. The actual
break was a separate, unmentioned incomplete-type error in a *new* test file.
When a CI job is red, `gh run view <id> --log` (or the Actions log) is the only
reliable source — grep for `error:` and the first failing TU, then read the
instantiation backtrace to find the incomplete/ill-formed type.

## 7. Leave one CPU free for local builds

Use `$(($(nproc)-1))` as the job count for local `make` or CMake builds. On this
four-CPU machine that means three build jobs, leaving one CPU for Codex and the
desktop app. For example:

```bash
cmake --build build --target amule -j "$(($(nproc)-1))"
```

---

# Lesson Learnt — eMuleBB→aMule port, PR split, branch rename (2026-10-23)

## 1. Re-check the current branch immediately before every commit
The user works in the same repo concurrently: they switched branches and
committed twice mid-session, and one of our commits landed on their branch
(`2037-move-kad-aich-prefs-to-files`). **Always run `git branch --show-current`
(or read `git status -sb`) right before committing** — the branch checked out
earlier in the session is not guaranteed. Recovery: `git cherry-pick` the stray
commit onto the correct branch, then `git branch -f` the user's branch back to
their last commit, and re-verify with `git log`.

## 2. A cross-repo PR needs a real fork — verify before pushing
`3togo/amule` is **not** a fork of `amule-org/amule` (`fork: false`), so PR
creation from it fails with `Validation Failed ... field "head", code "invalid"`.
The actual fork is `3togo/amule-upstream`. Check before planning the submission:

```bash
gh api repos/3togo/amule --jq '.fork, .parent.full_name'   # fork? parent?
gh api repos/amule-org/amule --jq '.permissions'           # push access?
```

## 3. Renaming a PR head branch CLOSES the PR — and it cannot be reopened
Using the branch-rename API on the fork closed all four cross-repo PRs;
`PATCH .../pulls/<n> {"state":"open"}` then fails with *"state cannot be
changed. The <old-branch> branch has been deleted."* The PR must be **recreated**
from the renamed branch. Decide functional branch names up front
(`<area>/<topic>`, e.g. `kad/peer-count-sanity`) — renaming after the fact costs
four new PRs plus supersession comments on the closed ones.

## 4. When `gh pr create` fails on GraphQL, fall back to REST
`gh pr create` failed with *"Head sha can't be blank, Base sha can't be blank,
No commits between ..."* even though the shas resolved fine via `gh api`. The
REST endpoint worked:

```bash
gh api -X POST repos/amule-org/amule/pulls \
  -f title="..." -f head="3togo:<branch>" -f base="master" -f body="..."
```

## 5. Any new `_()` string requires `po/amule.pot` in the same PR
The *"App catalogs in sync with source"* CI job runs `scripts/update-po.sh` +
`scripts/check-po-drift.sh`. One new translatable log line
(`AddLogLineN(CFormat(_("...")))`) failed it until the pot was regenerated and
committed as a separate `chore(po):` commit. `AddDebugLogLineN(...)` strings are
**not** translated — debug-only messages need no pot change.

## 6. Split into logical PRs by theme from the start
Four themes were cleanly separable because each touched different files:
server-connect / malformed-packet rejection / Kad+AICH guards / upload block
ownership. When changes are per-file, splitting is mechanical:

```bash
git checkout -b <new> <base> && git checkout <commit> -- <files> && git commit
```

## 7. Porting triage: most upstream-fork fixes do not apply to aMule
A 1642-commit delta yielded only ~7 portable fixes. aMule has diverged or
rewritten most of the shared code: `CCLIENTREF` lists instead of MFC `CList`,
`CMemFile` throwing `CEOFException` on overread (short packets already fail),
`CFileArea::ReadAt` + `CheckError()` for hashing, a flex lexer for IPFilter, no
`AICHSyncThread` at all, unbounded `EMSocket` queues (no overflow path), a
rewritten `CorruptionBlackBox`, and crypt-layer gating already present in
`CMuleUDPSocket::SendPacket`. **Verify the vulnerable pattern exists in the
target before porting** — and check the fix is not already there.

## 8. Validation on a shared machine: use a worktree, not the user's checkout
P4070-181's `~/git/amule` had the user's WIP, so the validation build ran in
`git worktree add /tmp/amule-port-build <branch>`; unit tests need
`-DBUILD_TESTING=ON` for `ctest` to find them; the worktree and logs were
removed afterwards (`git worktree remove --force`). Remote access is via the
jump host: `ssh -J root@100.126.179.3 polin@100.97.105.88` — direct
`3togo@100.97.105.88` publickey auth fails.

## 9. Stash WIP *including untracked files* before branch work
`git stash push -u -m ... -- . ':(exclude).codeartsdoer/'` (exclude the IDE
dir). Untracked files that also exist on the target branch block
`git checkout` — stash them too. Restore by switching back to the original
branch and popping. Note: a stashed untracked `LESSON_LEARNT.md` conflicts with
a later `stash pop` once the file is rewritten in the workspace — the workspace
copy is the live one.

## 10. Keep commit/PR text source-agnostic
When abstracting fixes from another project, describe each change on its own
merits (bug, fix, validation) and do not name the source project unless asked.

---

# Lesson Learnt — verifying what is actually submittable, and the submission route (2026-10-24)

## 1. Check merge state before planning any submission
`git status` said "ahead of origin by 4 commits", which reads as "unsubmitted".
It is not: `origin` is the personal mirror (`3togo/amule`), not upstream. PR
#2044 had **already merged** the branch's work as upstream commit `c3377359c`.
Before planning, always run `git log upstream/master --oneline -8` and
`gh pr list --repo amule-org/amule --state all --json number,title,headRefName,state`.

## 2. Use the two-dot diff to see what is outstanding
`git diff upstream/master...HEAD` (three-dot) diffs against the **merge base**,
so it shows a branch's own changes and makes an already-merged branch look
pending. `git diff upstream/master HEAD` (two-dot) shows the real delta versus
current upstream. Confirming the merge: `git diff upstream/master HEAD -- <the
PR's files>` came back empty for the Kad AICH files.

## 3. A stale working-tree workflow file can silently revert a merged fix
The uncommitted `.github/workflows/ccpp.yml` in the workspace was the **older**
collector-based variant of the runtime-deps fix; committing it would have undone
upstream's `dpkg-shlibdeps` implementation from #2058 (`c4ab38612`). Always diff
the working tree against `upstream/master` before committing anything under
`.github/`, and read the current step bodies rather than trusting a local copy.

## 4. Verify the tool pins from the workflows, not from memory
Upstream pins confirmed from the files: `LLVM_VERSION: '21'` (clang-tidy) and
`clang-format-version: '18'` — two separate settings. Local machine has
`clang-format-18` and `clang-tidy-18`, but **no** `clang-tidy` 21 and no plain
`clang-tidy`, so a local Tier-2 rerun is additional coverage, not an exact
reproduction of the CI gate.

## 5. The local `gh` token lacks `workflow`; P4070-181's has it
`gh auth status` locally: scopes `admin:public_key, gist, read:org, repo`.
On P4070-181 (user `polin`): the same plus **`workflow`**. Anything that pushes
or opens a PR touching `.github/workflows/**` must go through the machine whose
token carries `workflow`. A docs-only PR that adds one comment line per workflow
still counts.

## 6. `~/.bin/ts-ssh` cannot run a remote command
It accepts exactly one positional target, rejects extras ("only one target is
allowed"), and always execs an **interactive** login shell:
`ssh -tt -J <jump> user@target 'exec /bin/bash -ilc "exec /bin/bash -i"'`.
For non-interactive work replicate its route directly:
`ssh -o BatchMode=yes -J root@100.126.179.3 polin@100.97.105.88 'cmd'`.
The saved route lives in `~/.local/state/ts-ssh/last-successful`
(target/user/mode/jump), which is where `--last` reads it from.

## 7. The remote checkout holds the user's WIP — use a worktree there too
P4070-181's `~/git/amule` was on `gui/live-transfer-speeds` with dozens of
modified files. Fetch and worktree there (`git worktree add /tmp/... <branch>`),
never branch or commit in the live checkout. `git fetch upstream master` on the
main repo is safe (it does not touch the working tree).

## 8. Transfer work as a bundle, not a format-patch
`git bundle create x.bundle upstream/master..<branch>` (5 KB for one commit) +
`ssh ... 'cat > /tmp/x.bundle' < x.bundle`, then remotely
`git bundle verify` / `git fetch /tmp/x.bundle 'refs/heads/b:refs/heads/b'`.
The bundle's prerequisite is the commit's parent, so fetch `upstream master`
first. Deterministic, where `git am` of a patch can fuzz once the remote's base
has moved.

## 9. A guide must document the implementation that is actually in master
The drafted `CI_TROUBLESHOOTING.md` described `scripts/collect-runtime-deps.sh`
with its seven regression tests — an implementation upstream never took (#2058
merged `dpkg-shlibdeps` + `apt-get satisfy` instead). Rewrote the dependency
section to match master, and kept the collector's guarantees as a note for
branches that still carry it. A runbook that tells readers to run a script that
does not exist is worse than no runbook.

## 10. Keep internal notes out of upstream PRs
`LESSON_LEARNT.md` and `CI_NOTES.md` carry machine addresses, usernames, and
fork topology; `CI_TROUBLESHOOTING.md` is upstream-safe. Grep every doc before
it enters a PR: `grep -nE '100\.[0-9]+\.[0-9]+\.[0-9]+|polin|3togo|P4070|/home/|token|password'`.

## 11. Consent gate before submission
The user requires explicit consent before anything reaches `amule-org/amule`.
Prepare everything — branch, commit, PR title and body — present it, and wait.
Nothing was pushed to the fork and no PR was created while preparing this one.
