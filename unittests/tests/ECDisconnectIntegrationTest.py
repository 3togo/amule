#!/usr/bin/env python3
# Copyright (c) 2026 aMule Team
# SPDX-License-Identifier: GPL-2.0-or-later
"""Do not authenticate buffered packets after rejecting their EC connection."""
import hashlib
import os
from pathlib import Path
import socket
import struct
import subprocess
import sys
import tempfile

from AllSearchIntegrationTest import (C, connect_daemon, exact, free_ports, parse_tags,
                                      retry_port_collisions, stop_daemon, string, tag)
from DaemonConfig import EC_PASSWORD, write_amule_conf


def packet(opcode, tags=(), flags=0x20):
    body = bytes([C[opcode]]) + struct.pack('!H', len(tags)) + b''.join(tags)
    return struct.pack('!II', flags, len(body)) + body


def receive(peer):
    flags, length = struct.unpack('!II', exact(peer, 8))
    assert flags == 0x20, flags
    body = exact(peer, length)
    return body[0], parse_tags(body, 3, struct.unpack_from('!H', body, 1)[0])[0]


def challenge(peer):
    peer.sendall(packet('EC_OP_AUTH_REQ', [
        string(C['EC_TAG_CLIENT_NAME'], 'EC disconnect regression'),
        string(C['EC_TAG_CLIENT_VERSION'], '1'),
        tag(C['EC_TAG_PROTOCOL_VERSION'],
            struct.pack('!H', C['EC_CURRENT_PROTOCOL_VERSION']), 3),
        tag(C['EC_TAG_CAN_NOTIFY'])]))
    opcode, tags = receive(peer)
    assert opcode == C['EC_OP_AUTH_SALT'], (opcode, tags)
    salt = tags[C['EC_TAG_PASSWD_SALT']][0]
    password_hash = hashlib.md5(EC_PASSWORD.encode()).hexdigest()
    salt_hash = hashlib.md5(f'{salt:X}'.encode()).hexdigest()
    proof = hashlib.md5((password_hash + salt_hash).encode()).digest()
    return packet('EC_OP_AUTH_PASSWD', [tag(C['EC_TAG_PASSWD_HASH'], proof, 9)])


def run(binary):
    with tempfile.TemporaryDirectory(prefix='amule-ec-disconnect-') as tmp:
        root = Path(tmp)
        for directory in ('Incoming', 'Temp'):
            (root / directory).mkdir()
        ports = free_ports(3, udp_indices=(2,), peer_index=1)
        write_amule_conf(root, ports[0], Port=ports[1], UDPPort=ports[2])
        env = dict(os.environ, HOME=str(root), XDG_CONFIG_HOME=str(root / 'xdg'),
                   LC_ALL='C.UTF-8')
        env.pop('LD_PRELOAD', None)
        with (root / 'console.log').open('w') as console:
            proc = subprocess.Popen([binary, '-c', str(root), '--disable-fatal'],
                                    env=env, stdout=console, stderr=subprocess.STDOUT)
        control = None
        try:
            control = connect_daemon(proc, ports[0], root, ports[1], ports[2])
            # Positive control: the very same notification-capable login succeeds
            # when no rejected packet precedes its proof.
            with socket.create_connection(('127.0.0.1', ports[0]), timeout=10) as peer:
                peer.sendall(challenge(peer))
                assert receive(peer)[0] == C['EC_OP_AUTH_OK']
            assert control.call(C['EC_OP_STAT_REQ'])[0] == C['EC_OP_STATS']
            granted = (root / 'logfile').read_text().count('Access granted.')

            with socket.create_connection(('127.0.0.1', ports[0]), timeout=10) as peer:
                proof = challenge(peer)
                # One write puts the valid password proof and a command behind
                # a packet whose version flags ReadPacket must reject. The Asio
                # close leaves buffered input readable without a socket error.
                peer.sendall(packet('EC_OP_STAT_REQ', flags=0x60) + proof
                             + packet('EC_OP_STAT_REQ'))
                try:
                    assert peer.recv(1) == b'', 'dropped connection sent a reply'
                except ConnectionResetError:
                    pass

            # This round trip is a main-loop barrier, and would also expose a
            # notifier registered by the dying socket. No sleeps are needed.
            assert control.call(C['EC_OP_STAT_REQ'])[0] == C['EC_OP_STATS']
            assert proc.poll() is None, 'rejected batch killed the daemon'
            assert (root / 'logfile').read_text().count('Access granted.') == granted, \
                'buffered proof authenticated a dropped connection'

            # A packet after AUTH_FAIL must also tear the socket down: a plain local
            # close raises no lost event, so OnLost would never destroy it.
            closed = (root / 'logfile').read_text().count('External connection closed.')
            with socket.create_connection(('127.0.0.1', ports[0]), timeout=10) as peer:
                challenge(peer)
                wrong = packet('EC_OP_AUTH_PASSWD', [tag(C['EC_TAG_PASSWD_HASH'], bytes(16), 9)])
                peer.sendall(wrong + packet('EC_OP_STAT_REQ'))
                assert receive(peer)[0] == C['EC_OP_AUTH_FAIL']
                try:
                    assert peer.recv(1) == b'', 'failed login stayed open'
                except ConnectionResetError:
                    pass
            assert control.call(C['EC_OP_STAT_REQ'])[0] == C['EC_OP_STATS']
            assert (root / 'logfile').read_text().count('External connection closed.') \
                == closed + 1, 'packet after AUTH_FAIL left the socket alive'
            print('PASS: rejected EC batch cannot authenticate or register a notifier')
            print('PASS: packet after AUTH_FAIL closes and releases the connection')
            print('PASS: ordinary notification login and independent EC client still work')
        except BaseException:
            for name in ('logfile', 'console.log'):
                path = root / name
                if path.exists():
                    print(f'{name}:\n{path.read_text(errors="replace")[-8000:]}', file=sys.stderr)
            raise
        finally:
            if control is not None:
                control.sock.close()
            if proc.poll() is None:
                stop_daemon(proc)


if __name__ == '__main__':
    retry_port_collisions(run, str(Path(sys.argv[1]).resolve()))
