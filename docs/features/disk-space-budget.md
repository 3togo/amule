# Queue-wide disk headroom

When the queue checks disk space, active downloads reserve their remaining temp
file growth before insufficient-space downloads can resume. Resumes reserve
cumulatively in queue order, so two downloads cannot each spend the same free
space snapshot. Completion onto another volume reserves the full destination
copy too; same-volume completion needs no second copy reservation.

Each volume retains the existing minimum free-space floor (at least one part).
POSIX paths share a budget by device ID. Windows local mount paths share a budget
by volume GUID, with UNC share roots as the fallback. An unreadable volume/free
space snapshot prevents automatic resume rather than treating unknown space as
available. Ordinary paused/stopped downloads remain paused.

This is a conservative scheduling estimate, not filesystem preallocation. Other
processes can consume space after the snapshot, and copy/free-space changes are
rechecked at the existing interval. A completion already in progress can be
accounted conservatively for its full destination size.

DiskSpaceBudgetTest covers cumulative reservations, atomic cross-volume checks,
active demand, aliases, destination copies, floors and unknown volumes. Real
filesystem exhaustion and Windows mount aliases still require integration checks.
