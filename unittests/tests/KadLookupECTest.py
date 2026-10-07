#!/usr/bin/env python3
# Copyright (c) 2026 aMule Team
# SPDX-License-Identifier: GPL-2.0-or-later
"""Check Kad diagnostics through authenticated EC on an isolated loopback daemon."""
import hashlib
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
from AllSearchIntegrationTest import C, connect_daemon, free_port


def run(binary):
    with tempfile.TemporaryDirectory(prefix='amule-kad-diagnostics-') as directory:
        root = Path(directory)
        port = free_port()
        (root / 'amule.conf').write_text(f"""[eMule]
Nick=regression
Port={free_port()}
UDPPort={free_port()}
Address=127.0.0.1
ConnectToKad=0
ConnectToED2K=0
FilterLanIPs=0
NewVersionCheck=0
Reconnect=0
Serverlist=0
Ed2kServersUrl=
KadNodesUrl=
TempDir={root}/Temp
IncomingDir={root}/Incoming
[ExternalConnect]
AcceptExternalConnections=1
ECAddress=127.0.0.1
ECPort={port}
ECPassword={hashlib.md5(b'regression').hexdigest()}
""")
        env = dict(os.environ, XDG_CONFIG_HOME=str(root / 'xdg'))
        with (root / 'stdout.log').open('w') as log:
            proc = subprocess.Popen([binary, '-c', str(root)], stdout=log, stderr=log, env=env)
            try:
                ec = connect_daemon(proc, port)

                op, tags = ec.call(C['EC_OP_GET_KAD_LOOKUPS'])
                assert op == C['EC_OP_GET_KAD_LOOKUPS']
                text = tags[C['EC_TAG_STRING']][0].rstrip(b'\0').decode('utf-8')
                assert 'No Kad lookup history available' in text, text
                assert len(text.encode('utf-8')) < 65535
                # The diagnostic request leaves legacy search operations usable.
                assert ec.call(C['EC_OP_SEARCH_PROGRESS'])[0] == C['EC_OP_SEARCH_PROGRESS']
                ec.sock.close()
            except BaseException:
                log.flush()
                print((root / 'stdout.log').read_text(), file=sys.stderr)
                raise
            finally:
                if proc.poll() is None:
                    proc.terminate()
                try:
                    proc.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait()


if __name__ == '__main__':
    run(str(Path(sys.argv[1]).resolve()))
