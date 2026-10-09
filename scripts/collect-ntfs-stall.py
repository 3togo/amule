#!/usr/bin/env python3
r"""Collect Linux diagnostics for aMule issue #1588; Python 3, no extra packages.

Example (keep a download active and logs on ext4, not the NTFS mount):
  python3 collect-ntfs-stall.py /mnt/Transit --duration 180 \
      --notes 'ntfs-3g, one active download; manual trim after 30 seconds'
In another terminal, after about 30 seconds:
  sudo fstrim -v /mnt/Transit

This explicitly reproduces the suspected trigger even with X-fstrim.notrim.
The collector itself never trims, remounts, or writes to the tested filesystem.
Run a no-trim baseline first. Repeat with kernel ntfs3 only after stopping aMule
and fully unmounting/remounting the partition; keep the workload comparable.
An unsupported/failed trim is not a successful driver comparison.

Optional sudo provides /proc syscall/kernel stack and journal access. No aMule
passwords, configuration contents, or downloaded file contents are collected.
Review paths, process names, notes, and journal messages before sharing.
Send the printed .tar.gz report plus the manual fstrim output and workload details.
"""

import argparse
import datetime
import json
import os
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import time


def timestamp():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def emit(stream, **fields):
    stream.write(json.dumps(dict(time=timestamp(), **fields)) + "\n")
    stream.flush()


def read(path):
    try:
        return Path(path).read_text().strip()
    except OSError as error:
        return "unavailable: " + str(error)


def command(argv, timeout=10):
    # Avoid PIPE/communicate: after a timeout, waiting for a killed D-state
    # command to close its pipes could itself hang the collector indefinitely.
    try:
        with tempfile.TemporaryFile() as stdout, tempfile.TemporaryFile() as stderr:
            process = subprocess.Popen(argv, stdout=stdout, stderr=stderr)
            timed_out = False
            try:
                process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                timed_out = True
                process.kill()
                try:
                    process.wait(timeout=1)
                except subprocess.TimeoutExpired:
                    pass
            stdout.seek(0)
            stderr.seek(0)
            result = dict(command=argv, status=process.returncode,
                          stdout=stdout.read(262144).decode("utf-8", errors="replace"),
                          stderr=stderr.read(262144).decode("utf-8", errors="replace"))
            if timed_out:
                result.update(error=f"command exceeded {timeout} seconds",
                              pid=process.pid, awaiting_kernel_io=process.returncode is None)
            return result
    except OSError as error:
        return dict(command=argv, error=str(error))


def probe_worker(path):
    # Separate process: a blocked statfs must not block the state collector.
    while True:
        emit(sys.stdout, event="begin", monotonic=time.monotonic())
        started = time.monotonic()
        try:
            info = os.statvfs(path)
            emit(sys.stdout, event="end", elapsed_ms=(time.monotonic() - started) * 1000,
                 available_bytes=info.f_bavail * info.f_frsize)
        except OSError as error:
            emit(sys.stdout, event="end", elapsed_ms=(time.monotonic() - started) * 1000,
                 error=str(error))
        time.sleep(1)


def processes():
    wanted = {"amule", "amuled", "amuleapi", "mount.ntfs", "mount.ntfs-3g", "ntfs-3g"}
    result = []
    for directory in Path("/proc").iterdir():
        if not directory.name.isdecimal():
            continue
        name = read(directory / "comm")
        if name not in wanted:
            continue
        # Keep both AppImage launcher and real binary; executable paths identify them.
        try:
            executable = os.readlink(directory / "exe")
        except OSError as error:
            executable = "unavailable: " + str(error)
        result.append(dict(pid=int(directory.name), name=name, executable=executable,
                           stat=read(directory / "stat"), wchan=read(directory / "wchan"),
                           syscall=read(directory / "syscall"),
                           stack=read(directory / "stack"), io=read(directory / "io")))
    return sorted(result, key=lambda item: item["pid"])


def summarize(path, stopped_at):
    completed = []
    pending = None
    errors = 0
    for line in path.read_text().splitlines():
        try:
            event = json.loads(line)
        except ValueError:
            continue
        if event["event"] == "begin":
            pending = event
        else:
            completed.append(event["elapsed_ms"])
            errors += int("error" in event)
            pending = None
    return dict(completed_probes=len(completed), probe_errors=errors,
                max_completed_ms=max(completed, default=None),
                probes_over_3000_ms=sum(value > 3000 for value in completed),
                pending_probe=pending,
                pending_ms_at_stop=((stopped_at - pending["monotonic"]) * 1000
                                    if pending else None))


def positive_duration(value):
    number = int(value)
    if not 1 <= number <= 86400:
        raise argparse.ArgumentTypeError("duration must be between 1 and 86400 seconds")
    return number


def main():
    if len(sys.argv) == 3 and sys.argv[1] == "--probe-worker":
        probe_worker(sys.argv[2])
        return 0
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("mountpoint", help="filesystem path to probe, e.g. /mnt/Transit")
    parser.add_argument("--duration", type=positive_duration, default=180)
    parser.add_argument("--output", help="new report directory on ext4 or another local filesystem")
    parser.add_argument("--notes", default="", help="driver/test label and active download workload")
    args = parser.parse_args()
    if sys.platform != "linux":
        parser.error("this collector requires Linux /proc")
    mountpoint = os.path.abspath(args.mountpoint)
    # Inventory is captured before the reproduction; do not resolve the mount while stalled.
    mount = command(["findmnt", "-T", mountpoint, "-o", "SOURCE,TARGET,FSTYPE,OPTIONS", "--json"])
    if mount.get("status") != 0 or "error" in mount:
        parser.error("findmnt could not identify the test filesystem: " + str(mount))
    output = (Path(os.path.abspath(args.output)) if args.output else
              Path(tempfile.mkdtemp(prefix="amule-ntfs-")))
    if args.output:
        output.mkdir(mode=0o700, parents=True, exist_ok=False)
    location = command(["findmnt", "-T", str(output), "-n", "-o", "FSTYPE"])
    kind = location.get("stdout", "").strip().lower()
    if (location.get("status") != 0 or "error" in location or
            "ntfs" in kind or kind.startswith("fuse") or kind in {"nfs", "nfs4", "cifs", "smb3"}):
        output.rmdir()
        parser.error("put the report on a local non-NTFS, non-FUSE filesystem")
    started = timestamp()
    inventory = dict(started=started, mountpoint=mountpoint, notes=args.notes,
                     uid=os.geteuid(), duration_seconds=args.duration,
                     mount=mount, output_filesystem=location,
                     kernel=command(["uname", "-srvm"]), os_release=read("/etc/os-release"),
                     driver_version=command(["ntfs-3g", "--version"]),
                     trim_version=command(["fstrim", "--version"]),
                     packages=command(["dpkg-query", "-W", "ntfs-3g", "fuse", "fuse3", "util-linux"]),
                     devices=command(["lsblk", "-o", "NAME,TYPE,FSTYPE,MOUNTPOINTS,MODEL"]),
                     trim_service=command(["systemctl", "cat", "fstrim.service"]),
                     trim_timer=command(["systemctl", "list-timers", "fstrim.timer", "--no-pager"]))
    (output / "inventory.json").write_text(json.dumps(inventory, indent=2) + "\n")
    interrupted = False
    with (output / "probe.jsonl").open("w") as probe_log, \
            (output / "probe-stderr.txt").open("w") as probe_errors, \
            (output / "states.jsonl").open("w") as states:
        worker = subprocess.Popen([sys.executable, os.path.abspath(__file__),
                                   "--probe-worker", mountpoint],
                                  stdout=probe_log, stderr=probe_errors,
                                  start_new_session=True)
        print(f"Collecting for {args.duration} seconds. Report: {output}", flush=True)
        print("Keep the same download workload; run the planned trim in another terminal.", flush=True)
        deadline = time.monotonic() + args.duration
        try:
            while time.monotonic() < deadline:
                emit(states, processes=processes(), probe_pid=worker.pid,
                     probe_stat=read(f"/proc/{worker.pid}/stat"),
                     probe_wchan=read(f"/proc/{worker.pid}/wchan"),
                     cpu_pressure=read("/proc/pressure/cpu"),
                     io_pressure=read("/proc/pressure/io"),
                     memory_pressure=read("/proc/pressure/memory"),
                     diskstats=read("/proc/diskstats"))
                time.sleep(min(2, max(0, deadline - time.monotonic())))
        except KeyboardInterrupt:
            interrupted = True
        finally:
            stopped_at = time.monotonic()
            worker.kill()
            try:
                worker.wait(timeout=2)
                worker_pending = False
            except subprocess.TimeoutExpired:
                # Linux D-state calls cannot always be killed until the mount responds.
                worker_pending = True
    summary = summarize(output / "probe.jsonl", stopped_at)
    summary.update(finished=timestamp(), interrupted=interrupted,
                   worker_exit_status=worker.returncode,
                   worker_awaiting_kernel_io=worker_pending,
                   probe_failed=(worker.returncode not in (None, -9) or summary["probe_errors"] > 0))
    if worker_pending:
        summary["worker_pid"] = worker.pid
        print(f"Probe {worker.pid} is still in kernel I/O; it has been sent SIGKILL.", flush=True)
    (output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    journal = dict(trim=command(["journalctl", "-u", "fstrim.service", "--since", started,
                                 "--no-pager", "-o", "short-iso-precise", "-n", "200"]),
                   kernel=command(["journalctl", "-k", "--since", started, "--no-pager",
                                   "-o", "short-iso-precise", "-n", "200"]))
    (output / "journal.json").write_text(json.dumps(journal, indent=2) + "\n")
    archive = output.with_name(output.name + ".tar.gz")
    with tarfile.open(archive, "x:gz") as bundle:
        # Explicit list excludes unrelated files and late writes from a blocked probe.
        for name in ("inventory.json", "summary.json", "journal.json", "states.jsonl",
                     "probe.jsonl", "probe-stderr.txt"):
            bundle.add(output / name, arcname=output.name + "/" + name)
    print(json.dumps(summary, indent=2))
    print(f"Review and send: {archive}")
    print("Also send trim output/exit status and the matching aMule/amuleapi log excerpt.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
