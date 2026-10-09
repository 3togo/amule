#!/usr/bin/env python3
# Copyright (c) 2026 aMule Team
# SPDX-License-Identifier: GPL-2.0-or-later
"""Offline end-to-end user events: python3 UserEventsIntegrationTest.py AMULED AMULEGUI.

Requires DISPLAY and a local non-loopback IPv4 address. A tiny local peer
completes a real download. An intentionally oversized
free-space threshold raises OutOfDiskSpace within the normal 60-second check.
Two remote GUIs run their own commands, while the Core command stays daemon-local.
"""
import hashlib
import json
import os
from pathlib import Path
import socket
import socketserver
import struct
import subprocess
import sys
import tempfile
import threading
import time

from AllSearchIntegrationTest import C, EC, exact, integer, string, tag
from LogfilePathIntegrationTest import daemon_env, write_config


FILE_DATA = b'abc'
# RFC 1320 test vector: MD4("abc").
FILE_HASH = bytes.fromhex('a448017aaf21d8525fc10ae87aa6729d')


class LocalPeer(socketserver.ThreadingTCPServer):
    daemon_threads = True

    def __init__(self, handler, host="127.0.0.1"):
        super().__init__((host, 0), handler)
        self.worker = threading.Thread(target=self.serve_forever, daemon=True)
        self.worker.start()

    def close(self):
        self.shutdown()
        self.server_close()
        self.worker.join()


class WirePeer(socketserver.BaseRequestHandler):
    def send(self, opcode, payload=b''):
        body = bytes([opcode]) + payload
        self.request.sendall(struct.pack('<BI', 0xe3, len(body)) + body)

    def handle(self):
        self.request.settimeout(120)
        try:
            while True:
                protocol, length = struct.unpack('<BI', exact(self.request, 5))
                assert protocol == 0xe3, protocol
                packet = exact(self.request, length)
                self.respond(packet[0], packet[1:])
        except (EOFError, OSError):
            pass


class Server(WirePeer):
    def respond(self, opcode, payload):
        if opcode == 1:  # OP_LOGINREQUEST -> OP_IDCHANGE
            self.send(0x40, struct.pack('<II', 0x12345678, 0))


class DownloadPeer(WirePeer):
    def respond(self, opcode, payload):
        if opcode == 1:  # OP_HELLO -> OP_HELLOANSWER (plain eDonkey client)
            name = b'local event fixture'
            tags = (b'\x02\x01\x00\x01' + struct.pack('<H', len(name)) + name
                    + b'\x03\x01\x00\x11' + struct.pack('<I', 60))
            self.send(0x4c, bytes(range(1, 17)) + struct.pack('<IHI',
                struct.unpack('<I', socket.inet_aton(self.server.server_address[0]))[0],
                self.server.server_address[1], 2) + tags + struct.pack('<IH', 0, 0))
        elif opcode == 0x58:  # OP_REQUESTFILENAME -> OP_REQFILENAMEANSWER
            name = b'completed.txt'
            self.send(0x59, FILE_HASH + struct.pack('<H', len(name)) + name)
        elif opcode == 0x4f:  # OP_SETREQFILEID -> OP_FILESTATUS
            self.send(0x50, FILE_HASH + struct.pack('<H', 0))
        elif opcode == 0x51:  # OP_HASHSETREQUEST -> OP_HASHSETANSWER
            self.send(0x52, FILE_HASH + struct.pack('<H', 0))
        elif opcode == 0x54:  # OP_STARTUPLOADREQ -> OP_ACCEPTUPLOADREQ
            self.send(0x55)
        elif opcode == 0x47:  # OP_REQUESTPARTS -> OP_SENDINGPART
            assert payload[:16] == FILE_HASH
            ranges = struct.unpack('<6I', payload[16:40])
            for start, end in zip(ranges[:3], ranges[3:]):
                if end > start:
                    assert end <= len(FILE_DATA), ranges
                    self.send(0x46, FILE_HASH + struct.pack('<II', start, end) + FILE_DATA[start:end])


class EventEC(EC):
    def __init__(self, port):
        self.sock = socket.create_connection(('127.0.0.1', port), timeout=5)
        op, tags = self.call(C['EC_OP_AUTH_REQ'], [
            string(C['EC_TAG_CLIENT_NAME'], 'User events regression'),
            string(C['EC_TAG_CLIENT_VERSION'], '1'),
            tag(C['EC_TAG_PROTOCOL_VERSION'],
                struct.pack('!H', C['EC_CURRENT_PROTOCOL_VERSION']), 3),
            tag(C['EC_TAG_CAN_USER_EVENTS'])])
        assert op == C['EC_OP_AUTH_SALT'], (op, tags)
        salt = tags[C['EC_TAG_PASSWD_SALT']][0]
        secret = hashlib.md5(b'regression').hexdigest()
        proof = hashlib.md5((secret + hashlib.md5(f'{salt:X}'.encode()).hexdigest()).encode()).digest()
        op, tags = self.call(C['EC_OP_AUTH_PASSWD'], [tag(C['EC_TAG_PASSWD_HASH'], proof, 9)])
        assert op == C['EC_OP_AUTH_OK'], (op, tags)
        self.baseline = tags[C['EC_TAG_CAN_USER_EVENTS']][0]

    def events(self, after=None):
        op, tags = self.call(C['EC_OP_GET_USER_EVENTS'],
                             [] if after is None else [integer(C['EC_TAG_USER_EVENT_CURSOR'], after)])
        assert op == C['EC_OP_USER_EVENTS'], (op, tags)
        return tags


def run(daemon, gui):
    if not os.environ.get('DISPLAY'):
        print('SKIP: DISPLAY is required for the GUI smoke test')
        raise SystemExit(77)
    with tempfile.TemporaryDirectory(prefix='amule-user-events-') as temporary:
        root = Path(temporary)
        core = root / 'core'
        core.mkdir()
        port = write_config(core)
        config = core / 'amule.conf'
        config.write_text(config.read_text().replace('ConnectToED2K=0', 'ConnectToED2K=1').replace('[eMule]',
            '[eMule]\nMinFreeDiskSpace=0\nFilterLanIPs=0\nCheckDiskspace=1', 1)
            + f'\n[UserEvents/DownloadCompleted]\nCoreEnabled=1\nCoreCommand=/usr/bin/touch {core}/completed\n'
            + f'\n[UserEvents/OutOfDiskSpace]\nCoreEnabled=1\nCoreCommand=/usr/bin/touch {core}/marker\n')
        capture = root / 'capture.py'
        capture.write_text('from pathlib import Path\nimport json, sys\nPath(sys.argv[1]).write_text(json.dumps(sys.argv[2:]))\n')
        processes = []
        logs = []
        ec = None
        server = LocalPeer(Server)
        # aMule deliberately filters 127/8 download sources even with FilterLanIPs=0.
        # Bind only an address assigned to this host; no Internet peers are involved.
        host = next((address for address in socket.gethostbyname_ex(socket.gethostname())[2]
                     if not address.startswith('127.')), None)
        if host is None:
            server.close()
            print('SKIP: a local non-loopback IPv4 address is required for the download fixture')
            raise SystemExit(77)
        peer = LocalPeer(DownloadPeer, host)
        try:
            console = (core / 'console.log').open('w')
            logs.append(console)
            proc = subprocess.Popen([daemon, '-c', str(core), '--disable-fatal'],
                stdout=console, stderr=console, env=daemon_env(core))
            processes.append(proc)
            deadline = time.monotonic() + 120
            while True:
                assert proc.poll() is None, 'daemon exited during startup'
                try:
                    ec = EventEC(port)
                    break
                except ConnectionRefusedError:
                    assert time.monotonic() < deadline, 'EC listener did not start'
                    time.sleep(.1)
            assert C['EC_TAG_USER_EVENT'] not in ec.events(ec.baseline)
            for index in range(2):
                directory = root / f'gui{index}'
                directory.mkdir()
                (directory / 'remote.conf').write_text(f'''[eMule]
FirstRunWizardDone=1
NewVersionCheck=0
EnableTrayIcon=0
[EC]
Host=127.0.0.1
Port={port}
Password={hashlib.md5(b'regression').hexdigest()}
Encryption=0
[UserEvents/DownloadCompleted]
GUIEnabled=1
GUICommand=/usr/bin/python3 {capture} {directory}/completed "%FILE" "%NAME" "%HASH" "%SIZE" "%DLACTIVETIME"
[UserEvents/ErrorOnCompletion]
GUIEnabled=1
GUICommand=/usr/bin/touch {directory}/error
[UserEvents/OutOfDiskSpace]
GUIEnabled=1
GUICommand=/usr/bin/python3 {capture} {directory}/marker "%PARTITION"
''')
                log = (directory / 'console.log').open('w')
                logs.append(log)
                environment = daemon_env(directory)
                authority = os.environ.get('XAUTHORITY', str(Path.home() / '.Xauthority'))
                if Path(authority).exists():
                    environment['XAUTHORITY'] = authority
                processes.append(subprocess.Popen([gui, '-c', str(directory), '--skip', '--disable-fatal'],
                    stdout=log, stderr=log, env=environment))
            # Wait for both authenticated GUIs to finish initial setup and synchronize.
            deadline = time.monotonic() + 60
            while not all((root / f'gui{i}/remotelogfile').exists()
                          and 'Ready' in (root / f'gui{i}/remotelogfile').read_text()
                          for i in range(2)):
                assert time.monotonic() < deadline, 'remote GUIs did not finish startup'
                time.sleep(.1)
            time.sleep(2)
            assert ec.call(C['EC_OP_SERVER_ADD'], [string(C['EC_TAG_SERVER_ADDRESS'],
                f'localhost:{server.server_address[1]}')])[0] == C['EC_OP_NOOP']
            assert ec.call(C['EC_OP_SERVER_CONNECT'])[0] == C['EC_OP_NOOP']
            link = (f'ed2k://|file|completed.txt|3|{FILE_HASH.hex()}|/|sources,'
                    f'{peer.server_address[0]}:{peer.server_address[1]}|/')
            assert ec.call(C['EC_OP_ADD_LINK'], [string(C['EC_TAG_STRING'], link)])[0] == C['EC_OP_NOOP']
            completed = [root / f'gui{i}/completed' for i in range(2)]
            deadline = time.monotonic() + 90
            while not all(marker.exists() for marker in completed):
                assert all(process.poll() is None for process in processes), 'an application exited'
                assert time.monotonic() < deadline, 'download completion commands did not fire'
                time.sleep(.2)
            assert (core / 'Incoming/completed.txt').read_bytes() == FILE_DATA
            assert (core / 'completed').exists(), 'Core completion command did not fire'
            for marker in completed:
                values = json.loads(marker.read_text())
                assert values[:4] == [str(core / 'Incoming/completed.txt'), 'completed.txt',
                                      FILE_HASH.hex().upper(), '3'], values
                assert values[4], values
                assert not (marker.parent / 'error').exists()
            print('PASS: real local download completes and both GUIs receive every completion variable')
            # Raise the threshold without exhausting disk space or restarting the daemon.
            assert ec.call(C['EC_OP_SET_PREFERENCES'], [tag(C['EC_TAG_PREFS_FILES'], children=[
                integer(C['EC_TAG_FILES_MIN_FREE_SPACE'], 1000000000)])])[0] == C['EC_OP_NOOP']
            deadline = time.monotonic() + 90
            markers = [root / f'gui{index}/marker' for index in range(2)]
            while not all(marker.exists() for marker in markers):
                assert all(process.poll() is None for process in processes), 'an application exited'
                assert time.monotonic() < deadline, 'GUI commands did not fire'
                time.sleep(.2)
            assert (core / 'marker').exists(), 'daemon-local Core command did not fire'
            for marker in markers:
                assert json.loads(marker.read_text()) == [str(core / 'Temp')], marker.read_text()
            events = ec.events(ec.baseline)
            event_id, children = events[C['EC_TAG_USER_EVENT']]
            assert event_id > ec.baseline
            assert children[C['EC_TAG_USER_EVENT_KEY']][0] == b'OutOfDiskSpace\0'
            assert C['EC_TAG_USER_EVENT'] not in ec.events(event_id)
            assert C['EC_TAG_USER_EVENT'] not in ec.events(), 'synchronizing returned history'
            fresh = EventEC(port)
            try:
                assert C['EC_TAG_USER_EVENT'] not in fresh.events(0), 'new login replayed old events'
            finally:
                fresh.sock.close()
            print('PASS: daemon event reaches both GUIs and expands the daemon path')
            print('PASS: Core command remains daemon-local; polling and new login do not replay')
        except BaseException:
            for log in logs:
                log.flush()
            for file in list(root.rglob('console.log')) + list(root.rglob('logfile')):
                print(f'{file}:\n{file.read_text()}', file=sys.stderr)
            raise
        finally:
            if ec:
                ec.sock.close()
            for process in reversed(processes):
                if process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=10)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait()
            peer.close()
            server.close()
            for log in logs:
                log.close()


if __name__ == '__main__':
    run(*(str(Path(argument).resolve()) for argument in sys.argv[1:]))
