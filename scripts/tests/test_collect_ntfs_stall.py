"""Check that diagnostic collection survives filesystem and helper stalls."""

import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "collect-ntfs-stall.py"
SPEC = importlib.util.spec_from_file_location("collector", SCRIPT)
COLLECTOR = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(COLLECTOR)


@unittest.skipUnless(sys.platform == "linux", "requires Linux /proc")
class CollectorTest(unittest.TestCase):
    def test_helper_timeout_is_reported(self):
        result = COLLECTOR.command([sys.executable, "-c", "import time; time.sleep(60)"],
                                   timeout=0.1)
        self.assertIn("error", result)
        self.assertFalse(result["awaiting_kernel_io"])

    def test_missing_helper_is_reported(self):
        result = COLLECTOR.command(["/nonexistent/amule-diagnostic-command"])
        self.assertIn("error", result)

    def collect(self, root, blocked=False, fail=False):
        # Run the actual collector and child lifecycle. Only the probe call and
        # inventory helpers are replaced; no driver, mount or trim is required.
        wrapper = root / "runner.py"
        wrapper.write_text(
            "import importlib.util, sys, time\n"
            f"spec = importlib.util.spec_from_file_location('collector', {str(SCRIPT)!r})\n"
            "module = importlib.util.module_from_spec(spec)\n"
            "spec.loader.exec_module(module)\n"
            "module.__file__ = __file__\n"
            "module.command = lambda argv: dict(command=argv, status=0, stdout='ext4', stderr='')\n"
            + ("if '--probe-worker' in sys.argv:\n"
               "    module.os.statvfs = lambda path: time.sleep(60)\n" if blocked else "")
            + ("if '--probe-worker' in sys.argv:\n"
               "    sys.exit(7)\n" if fail else "")
            + "sys.exit(module.main())\n"
        )
        report = root / "report"
        result = subprocess.run([sys.executable, str(wrapper), str(root), "--duration", "3",
                                 "--output", str(report)], capture_output=True, text=True,
                                timeout=15)
        self.assertEqual(result.returncode, 0, result.stderr)
        summary = json.loads((report / "summary.json").read_text())
        self.assertGreaterEqual(len((report / "states.jsonl").read_text().splitlines()), 2)
        with tarfile.open(root / "report.tar.gz") as bundle:
            self.assertEqual({Path(name).name for name in bundle.getnames()},
                             {"inventory.json", "summary.json", "journal.json", "states.jsonl",
                              "probe.jsonl", "probe-stderr.txt"})
        return summary

    def test_normal_probe(self):
        with tempfile.TemporaryDirectory() as directory:
            summary = self.collect(Path(directory))
        self.assertGreater(summary["completed_probes"], 0)
        self.assertIsNone(summary["pending_probe"])
        self.assertFalse(summary["probe_failed"])

    def test_blocked_probe_does_not_stop_collection(self):
        with tempfile.TemporaryDirectory() as directory:
            summary = self.collect(Path(directory), blocked=True)
        self.assertEqual(summary["completed_probes"], 0)
        self.assertGreater(summary["pending_ms_at_stop"], 1000)

    def test_failed_worker_is_not_reported_as_success(self):
        with tempfile.TemporaryDirectory() as directory:
            summary = self.collect(Path(directory), fail=True)
        self.assertEqual(summary["worker_exit_status"], 7)
        self.assertTrue(summary["probe_failed"])


if __name__ == "__main__":
    unittest.main()
