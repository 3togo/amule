#!/usr/bin/env python3
# Copyright (c) 2026 aMule Team
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise AllSearch through EC against an isolated daemon and a loopback eD2k server."""
import hashlib
import os
from pathlib import Path
import queue
import socket
import struct
import subprocess
import sys
import tempfile
import threading
import time


def exact(sock, n):
    data = b''
    while len(data) < n:
        part = sock.recv(n - len(data))
        if not part:
            raise EOFError('connection closed')
        data += part
    return data


def tag(name, value=b'', kind=1, children=()):
    nested = b''.join(children)
    return (struct.pack('!HBI', name * 2 + bool(children), kind, len(value) + len(nested))
            + (struct.pack('!H', len(children)) if children else b'') + nested + value)


def string(name, value):
    return tag(name, value.encode() + b'\0', 6)


def integer(name, value):
    return tag(name, struct.pack('!I', value), 4)


def parse_tags(data, offset, count):
    result = {}
    for _ in range(count):
        name, kind, length = struct.unpack_from('!HBI', data, offset)
        offset += 7
        children = {}
        if name & 1:
            n = struct.unpack_from('!H', data, offset)[0]
            offset += 2
            start = offset
            children, offset = parse_tags(data, offset, n)
            length -= offset - start
        value = data[offset:offset + length]
        offset += length
        if kind in (2, 3, 4, 5):
            value = int.from_bytes(value, 'big')
        result[name >> 1] = (value, children)
    return result, offset


class EC:
    def __init__(self, port):
        self.sock = socket.create_connection(('127.0.0.1', port), timeout=5)
        op, tags = self.call(2, [string(0x100, 'AllSearch regression'), string(0x101, '1'),
                               tag(2, b'\x02\x04', 3), tag(0x15), tag(0x1a)])
        assert op == 0x4f, (op, tags)
        salt = tags[0xb][0]
        password_hash = hashlib.md5(b'regression').hexdigest()
        salt_hash = hashlib.md5(f'{salt:X}'.encode()).hexdigest()
        proof = hashlib.md5((password_hash + salt_hash).encode()).digest()
        op, tags = self.call(0x50, [tag(1, proof, 9)])
        assert op == 4 and 0x29 in tags, (op, tags)

    def call(self, op, tags=()):
        payload = bytes([op]) + struct.pack('!H', len(tags)) + b''.join(tags)
        self.sock.sendall(struct.pack('!II', 0x20, len(payload)) + payload)
        flags, length = struct.unpack('!II', exact(self.sock, 8))
        assert flags == 0x20, flags
        reply = exact(self.sock, length)
        return reply[0], parse_tags(reply, 3, struct.unpack_from('!H', reply, 1)[0])[0]

    def start(self, query, kind=5, wait=False):
        request = tag(0x701, bytes([kind]), 2, [string(0x702, query), string(0x705, '')])
        for _ in range(100 if wait else 1):
            op, tags = self.call(0x26, [request])
            if op == 6:
                break
            time.sleep(0.1)
        assert op == 6, (op, tags)
        return tags[0x70e][0]

    def progress(self, sid):
        return self.call(0x29, [integer(0x70e, sid)])[1]


def connect_daemon(proc, port):
    for _ in range(100):
        if proc.poll() is not None:
            raise RuntimeError('daemon exited')
        try:
            return EC(port)
        except ConnectionRefusedError:
            time.sleep(0.1)
    raise TimeoutError('EC listener did not start')


def search_record(name):
    name = name.encode()
    tags = (b'\x02\x01\x00\x01' + struct.pack('<H', len(name)) + name
            + b'\x03\x01\x00\x02' + struct.pack('<I', 4096))
    return bytes(range(16)) + struct.pack('<IHI', 0, 0, 2) + tags


def stop_daemon(proc):
    proc.terminate()
    code = proc.wait(timeout=10)
    assert code == 0, f"daemon shutdown failed: {code}"


def free_port():
    with socket.socket() as s:
        s.bind(('127.0.0.1', 0))
        return s.getsockname()[1]


def run(binary):
    with tempfile.TemporaryDirectory(prefix='amule-all-search-') as root:
        root = Path(root)
        ec_port = free_port()
        config = f'''[eMule]
Nick=regression
Port={free_port()}
UDPPort={free_port()}
Address=127.0.0.1
ConnectToKad=1
ConnectToED2K=1
FilterLanIPs=0
NewVersionCheck=0
Reconnect=0
Serverlist=0
Ed2kServersUrl=
KadNodesUrl=
AddServerListFromServer=0
AddServerListFromClient=0
TempDir={root}/Temp
IncomingDir={root}/Incoming
[ExternalConnect]
AcceptExternalConnections=1
ECAddress=127.0.0.1
ECPort={ec_port}
ECPassword={hashlib.md5(b'regression').hexdigest()}
'''
        (root / 'amule.conf').write_text(config)
        (root / 'nodes.dat').write_bytes(struct.pack('<III', 0, 1, 0))
        env = dict(os.environ, HOME=str(root), XDG_CONFIG_HOME=str(root / 'xdg'))
        with (root / 'stdout.log').open('w') as log:
            proc = subprocess.Popen([binary, '-c', str(root)], stdout=log, stderr=log, env=env)
            try:
                ec = connect_daemon(proc, ec_port)
                # No available network: fail without creating a search.
                assert ec.call(0x49)[0] == 1
                op, _ = ec.call(0x26, [tag(0x701, b'\x05', 2, [string(0x702, 'ubuntu')])])
                assert op == 5, op
                # Kad-only fallback must not wait for a nonexistent server response.
                assert ec.call(0x48)[0] == 1
                sid = ec.start('ubuntu linux')
                state = ec.progress(sid)
                assert state[0x70b][0] == 5 and state[0x717][0] == 1, state
                assert state[0x70a][0] == 1, state
                # A duplicate target must reject the new request without deleting this one.
                op, _ = ec.call(0x26, [tag(0x701, b'\x05', 2,
                    [string(0x702, 'ubuntu linux'), string(0x705, '')])])
                assert op == 5 and ec.progress(sid)[0x717][0] == 1
                assert ec.call(0x27, [integer(0x70e, sid)])[0] == 7
                state = ec.progress(sid)
                assert state[0x70a][0] == 2 and state[0x717][0] == 0, state
                # Closing must remove its Kad target so the same keyword can restart.
                sid = ec.start('debian testing')
                assert ec.call(0x27, [integer(0x70e, sid), tag(0x711)])[0] == 7
                sid = ec.start('debian testing')
                assert ec.progress(sid)[0x717][0] == 1
                ec.call(0x27, [integer(0x70e, sid)])
                assert ec.call(0x49)[0] == 1
                # Loopback server checks that All keeps the full eD2k query.
                with socket.socket() as listener:
                    listener.bind(('127.0.0.1', 0))
                    listener.listen(1)
                    listener.settimeout(10)
                    port = listener.getsockname()[1]
                    queries = queue.Queue()
                    answer = threading.Event()
                    answer.set()
                    def serve():
                        try:
                            with listener.accept()[0] as peer:
                                peer.settimeout(10)
                                while True:
                                    proto, length = struct.unpack('<BI', exact(peer, 5))
                                    packet = exact(peer, length)
                                    if packet[0] == 1:  # OP_LOGINREQUEST
                                        body = b'\x40' + struct.pack('<II', 0x12345678, 0)
                                        peer.sendall(struct.pack('<BI', 0xe3, len(body)) + body)
                                    elif packet[0] == 0x16:  # OP_SEARCHREQUEST
                                        queries.put(packet[1:])
                                        if not answer.wait(10):
                                            return
                                        body = (b'\x33' + struct.pack('<I', 3)
                                                + search_record('regression.bin')
                                                + search_record('variant.bin')
                                                + search_record('variant.bin'))
                                        peer.sendall(struct.pack('<BI', 0xe3, len(body)) + body)
                        except (EOFError, OSError):
                            pass
                    worker = threading.Thread(target=serve, daemon=True)
                    worker.start()
                    assert ec.call(0x31, [string(0x503, f'localhost:{port}')])[0] == 1
                    assert ec.call(0x2f)[0] == 1
                    sid = ec.start('ubuntu linux', wait=True)
                    query = queries.get(timeout=5)
                    assert b'ubuntu' in query and b'linux' in query, query
                    assert ec.progress(sid)[0x717][0] == 0
                    ec.call(0x27, [integer(0x70e, sid)])
                    # Both components: eD2k completion cannot finish the Kad component.
                    assert ec.call(0x48)[0] == 1
                    sid = ec.start('fedora workstation')
                    query = queries.get(timeout=5)
                    assert b'fedora' in query and b'workstation' in query, query
                    time.sleep(2)
                    state = ec.progress(sid)
                    assert state[0x70a][0] == 1 and state[0x717][0] == 1, state
                    # Finishing another Kad search must not complete this combined one.
                    other = ec.start('opensuse tumbleweed', kind=2)
                    ec.call(0x27, [integer(0x70e, other)])
                    assert ec.progress(sid)[0x70a][0] == 1
                    ec.call(0x27, [integer(0x70e, sid)])
                    assert ec.progress(sid)[0x70a][0] == 2
                    # Reverse completion order: hold the server answer while Kad stops.
                    answer.clear()
                    sid = ec.start('alpine linux')
                    queries.get(timeout=5)
                    assert ec.call(0x49)[0] == 1
                    state = ec.progress(sid)
                    assert state[0x70a][0] == 1 and state[0x717][0] == 0, state
                    answer.set()
                    for _ in range(50):
                        state = ec.progress(sid)
                        if state[0x70a][0] == 2:
                            break
                        time.sleep(0.1)
                    assert state[0x70a][0] == 2, state
                    assert state[0x70c][0] == 1, state
                    ec.sock.close()
                # Persist and reload an All search: its finished Kad marker must not
                # make it appear to be a standalone Kad search after restart.
                stop_daemon(proc)
                worker.join(timeout=2)
                proc = subprocess.Popen([binary, '-c', str(root)], stdout=log, stderr=log, env=env)
                ec = connect_daemon(proc, ec_port)
                state = ec.progress(sid)
                assert state[0x70b][0] == 5, state
                assert state[0x70a][0] == 2 and state[0x717][0] == 0, state
                # Repeated close/restart and bulk shutdown exercise registry ownership.
                ec.call(0x27, [integer(0x70e, sid), tag(0x711)])
                assert ec.call(0x48)[0] == 1
                for i in range(30):
                    current = ec.start(f'ownership{i} regression', kind=2)
                    ec.call(0x27, [integer(0x70e, current), tag(0x711)])
                    current = ec.start(f'ownership{i} regression', kind=2)
                    ec.call(0x27, [integer(0x70e, current), tag(0x711)])
                active = [ec.start(f'bulkownership{i} regression', kind=2) for i in range(20)]
                assert ec.call(0x49)[0] == 1
                for current in active:
                    assert ec.progress(current)[0x70a][0] == 2
                assert ec.call(0x48)[0] == 1
                for i in range(20):
                    ec.start(f'shutdownownership{i} regression', kind=2)
                ec.sock.close()
                stop_daemon(proc)
                print('PASS: network fallback, query encoding, both completion orders, '
                      'duplicate targets, stop/close, persistence, ownership stress, clean shutdown')
            except BaseException:
                log.flush()
                print((root / 'stdout.log').read_text(), file=sys.stderr)
                if (root / 'logfile').exists():
                    print((root / 'logfile').read_text()[-8000:], file=sys.stderr)
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
