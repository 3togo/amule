#!/usr/bin/env python3
# Copyright (c) 2026 aMule Team
# SPDX-License-Identifier: GPL-2.0-or-later
"""Check Kad diagnostics through authenticated EC on an isolated loopback daemon."""
import hashlib
import os
from pathlib import Path
import struct
import socket
import time
import subprocess
import sys
import tempfile
from AllSearchIntegrationTest import C, connect_daemon, free_port


def local_peer():
    try:
        addresses = socket.getaddrinfo(socket.gethostname(), None, socket.AF_INET, socket.SOCK_DGRAM)
    except OSError:
        return None
    for address in addresses:
        ip = address[4][0]
        octets = tuple(map(int, ip.split('.')))
        if octets[0] in (10, 127) or octets[:2] == (192, 168) or (octets[0] == 172 and 16 <= octets[1] <= 31):
            continue
        peer = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            peer.bind((ip, 0))
            return peer
        except OSError:
            peer.close()
    return None


def run(binary, populated=False):
    peer = local_peer() if populated else None
    if populated and peer is None:
        print('No owned Kad-compatible IPv4 address available')
        return 77
    with tempfile.TemporaryDirectory(prefix='amule-kad-diagnostics-') as directory:
        root = Path(directory)
        port = free_port()
        if peer:
            ip, udp = peer.getsockname()
            contact = struct.pack('<4IIHHB2IB', 0xA0000000, 0, 0, 1,
                                  int.from_bytes(socket.inet_aton(ip), 'big'), udp, udp, 8, 0, 0, 1)
            (root / 'nodes.dat').write_bytes(struct.pack('<III', 0, 2, 1) + contact)
        (root / 'amule.conf').write_text(f"""[eMule]
Nick=regression
Port={free_port()}
UDPPort={free_port()}
Address=127.0.0.1
ConnectToKad=1
Autoconnect=0
ConnectToED2K=0
FilterLanIPs=0
NewVersionCheck=0
GeoIPEnabled=0
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
                if peer:
                    assert ec.call(C['EC_OP_KAD_START'])[0] == C['EC_OP_NOOP']
                    sid = ec.start('lookup lifecycle regression', kind=C['EC_SEARCH_KAD'])
                    endpoint = f'{ip}:{udp}'
                    def snapshot():
                        return ec.call(C['EC_OP_GET_KAD_LOOKUPS'])[1][C['EC_TAG_STRING']][0].rstrip(b'\0').decode('utf-8')
                    deadline = time.monotonic() + 10
                    while endpoint not in snapshot() and time.monotonic() < deadline:
                        time.sleep(0.1)
                    active = snapshot()
                    assert endpoint in active and 'routing requests' in active and 'active' in active, active
                    # The actual production search must archive its value snapshot on stop.
                    from AllSearchIntegrationTest import integer
                    assert ec.call(C['EC_OP_SEARCH_STOP'], [integer(C['EC_TAG_SEARCH_ID'], sid)])[0] == C['EC_OP_MISC_DATA']
                    assert 'finished' in snapshot(), snapshot()
                    ec.sock.close()
                    ec = connect_daemon(proc, port)
                    assert endpoint in snapshot(), snapshot()
                    assert ec.call(C['EC_OP_KAD_STOP'])[0] == C['EC_OP_NOOP']
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
                if peer:
                    peer.close()


if __name__ == '__main__':
    sys.exit(run(str(Path(sys.argv[1]).resolve()), '--populated' in sys.argv) or 0)
