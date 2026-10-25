# Fixing aMule CI failures

Start with the failing job's log and the current PR revision. Do not diagnose
from the PR discussion, a stale check, or the last error line alone.

Detailed instructions and known fix revisions are in
[the workflow troubleshooting guide](.github/workflows/CI_TROUBLESHOOTING.md).
This note is the quick checklist for future fixes.

## 1. Establish what actually failed

Replace the uppercase placeholders with the PR, run, and job numbers:

```sh
git status --short --branch
gh pr view PR_NUMBER --repo amule-org/amule --json headRefName,headRefOid
gh pr checks PR_NUMBER --repo amule-org/amule
gh api repos/amule-org/amule/actions/jobs/JOB_ID
gh api repos/amule-org/amule/actions/jobs/JOB_ID/logs > /tmp/amule-ci-job.log
gh run view RUN_ID --repo amule-org/amule --log-failed
```

Read the first failed step, its timestamps, and the first useful diagnostic.
Distinguish compilation, lint, tests, dependency downloads, and runner allocation.
For reused artifacts, record the original CI merge revision as well as the PR
head; they can differ.

## 2. Apply the fix for that failure class

| Symptom | Diagnosis and fix |
| --- | --- |
| Smoke job cancelled while apt is downloading; daemon/tests skipped | The tests never ran. A run-only job should install binary-derived runtime packages, not the whole build dependency list. Keep the producer's manifest, artifact tar, and consumer install in sync. Use download retries and separate finite job/test timeouts. |
| Runtime collector emits invalid package names or misses libraries | Handle transitive libraries, the loader, merged-/usr paths, and multiarch owners. Ignore `dpkg-query` diversion descriptions. Fail on unresolved or unowned libraries and failed/empty discovery. Run the collector regression tests. |
| `modernize-use-nullptr` on a changed source line | Replace pointer uses of `NULL` with `nullptr`; retain the required guard or behavior. Check the entire PR diff for the same copied pattern. A successful build does not establish that lint passes. |
| Formatting failure | Run the pinned formatter on complete affected C++ files and inspect the diff. Formatting only the newly edited lines can leave the file failing CI. |
| clang-tidy config reports an unknown key | Check the actual LLVM version. For supplemental LLVM 18 checks, remove only `ExcludeHeaderFilterRegex` from an external temporary config, keeping repository configs intact. |
| Incomplete type in a test stub or missing `upnp.h` | Match CI feature flags. Out-of-line destructors need complete member types. Include the defining header under the matching feature guard and give the test target the imported dependency's include directories. |
| Catalog drift after adding translated text | Regenerate translations with the repository's `scripts/update-po.sh` workflow and check with `scripts/check-po-drift.sh`. Include the required catalog changes in the PR. |
| Timeout, address-in-use, or intermittent authentication failure | Reproduce with isolated profiles, ports, files, and processes. Fixed `/tmp` paths and process-killing patterns require separate containers or serialized runs. Determine whether the cause is a hang, interference, or the wrong test budget before changing a timeout. |
| Workflow push rejected for missing OAuth `workflow` scope | Use the repository's already-authorized SSH push route when available. Changing source or rerunning tests cannot resolve an authentication-scope error. |

## 3. Match the environment and validate the candidate

- Read the current workflow pins before selecting tools. On 2026-10-23,
  clang-format was pinned to **18**, while CI clang-tidy used **LLVM 21**.
  Separately requested LLVM 18 checks are additional coverage, not an exact
  reproduction of the LLVM 21 gate.
- Configure with the same relevant feature flags and generate a matching
  `compile_commands.json`. For clang-tidy, check affected translation units
  with Tier-1 and all changed ranges against CI's merge base with Tier-2.
  Inspect diagnostics: a zero tool exit code can still accompany warnings.
- For runtime package changes, validate on the workflow's distribution. A
  newer development host's package names and libraries do not prove compatibility
  with an older CI image. Use the configured remote development environment for
  substantial builds and tests, following the global access instructions.
- Run the affected build/tests first, then any broader suite required by the
  change. Use `--no-tests=error` for a targeted CTest selection so a misspelled
  expression cannot silently pass. Keep daemon tests offline and disposable.

Examples for the failures investigated here, from a checkout containing the
complete runtime fix and a matching configured build:

```sh
clang-format-18 --dry-run --Werror \
  src/SHAHashSet.cpp src/kademlia/net/KademliaUDPListener.cpp
python3 scripts/tests/test_collect_runtime_deps.py
bash -n scripts/collect-runtime-deps.sh
ctest --test-dir build --output-on-failure --no-tests=error \
  -R '^(AICHSearchResultTest|KadAICHHashListTest)$'
```

These commands are focused examples, not a replacement for the failing CI job.
If the collector or its tests are absent on an older branch, inspect the
reference fixes in the detailed guide and port the complete producer/consumer
change before validating it. Follow that guide for the full offline smoke run.

## 4. Prevent the same fix from disappearing again

Before rebasing, porting, or resolving a workflow conflict, compare the affected
sections with the known working revision. Do not replace an entire workflow
with an older copy to resolve one conflict. A fix being an ancestor does not
prove it remains effective: later commits can undo it.

Keep regression tests and their workflow path filters with the fix. Verify
runtime manifests are both packaged and consumed. Retain lint checks instead
of adding suppressions for simple source corrections.

Work in an isolated checkout if the current one has unrelated changes. Stage
only intended files, inspect the staged diff, and recheck the branch before
committing. Verify the pushed PR head afterward.

## 5. Record the result precisely

Save the exact source SHA and patch identity, verified OS/toolchain, commands,
passes/failures/skips, and any coverage limits. For downloaded binaries, save
artifact hashes and their source revision; do not describe them as fresh builds.
Distinguish a passing remote reproduction from GitHub checks still in progress.

Keep credentials, signing material, machine addresses, account names, and
private access logs outside repository documentation and commits.
