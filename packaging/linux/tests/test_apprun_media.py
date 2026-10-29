"""The extracted AppImage must dispatch helper invocations to the helper."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

SOURCE = Path(__file__).resolve().parents[3]


class AppRunMediaTests(unittest.TestCase):
    def test_helper_invocation_uses_helper_instead_of_gui(self):
        with tempfile.TemporaryDirectory(prefix='amule relocated AppDir ') as scratch:
            root = Path(scratch)
            shutil.copy2(SOURCE / 'packaging/linux/appimage/AppRun', root / 'AppRun')
            (root / 'AppRun').chmod(0o755)
            for binary, folder, output in (('amule-media-probe', 'usr/libexec/amule', 'helper'),
                                           ('amule', 'usr/bin', 'gui')):
                (root / folder).mkdir(parents=True, exist_ok=True)
                path = root / folder / binary
                path.write_text('#!/bin/sh\nprintf "%s %s\\n" ' + output + ' "$1"\n')
                path.chmod(0o755)
            env = dict(os.environ, ARGV0='/relocated/amule-media-probe.AppImage')
            env.pop('LD_PRELOAD', None)
            reply = subprocess.run([str(root / 'AppRun'), '--version'], env=env,
                                   text=True, capture_output=True, check=True)
            self.assertEqual(reply.stdout, 'helper --version\n')


if __name__ == '__main__':
    unittest.main()
