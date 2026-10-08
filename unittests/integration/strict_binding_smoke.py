#!/usr/bin/env python3
"""A missing explicit interface must stop the daemon before it opens listeners."""
import argparse
import hashlib
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--amuled', required=True)
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='amule-bind-test-') as directory:
    root = Path(directory)
    (root / 'incoming').mkdir()
    (root / 'temp').mkdir()
    (root / 'amule.conf').write_text(f'''[eMule]
NetworkInterface=amule-no-such-interface
Language=en
Autoconnect=0
ConnectToKad=0
ConnectToED2K=0
NewVersionCheck=0
UDPDisable=1
TempDir={root / 'temp'}
IncomingDir={root / 'incoming'}
[ExternalConnect]
AcceptExternalConnections=1
ECAddress=127.0.0.1
ECPassword={hashlib.md5(b'disposable-password').hexdigest()}
[WebServer]
Enabled=0
[AmuleApi]
Enabled=0
''')
    result = subprocess.run([args.amuled, '-c', directory, '-o'],
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            text=True, timeout=15)
    assert result.returncode > 0, result.stdout
    assert 'startup blocked' in result.stdout, result.stdout
    assert 'listening on' not in result.stdout, result.stdout
    assert 'fatal error' not in result.stdout.lower(), result.stdout
print('strict interface startup smoke test passed')
