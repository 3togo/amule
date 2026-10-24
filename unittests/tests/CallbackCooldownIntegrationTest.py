#!/usr/bin/env python3
# Copyright (c) 2026 aMule Team
# SPDX-License-Identifier: GPL-2.0-or-later
"""Malformed direct UDP callbacks must retain the existing per-address cooldown."""
from pathlib import Path
import socket
import struct
import sys
import tempfile

from AllSearchIntegrationTest import C, free_ports, retry_port_collisions, stop_daemon
from DaemonConfig import write_amule_conf
from ShortBlockPacketIntegrationTest import start


def callback(udp, port, listener):
    data = struct.pack('<H', listener.getsockname()[1]) + bytes(range(16)) + b'\0'
    udp.sendto(b'\xc5\x95' + data, ('127.0.0.1', port))


def run(binary):
    with tempfile.TemporaryDirectory(prefix='amule-callback-cooldown-') as directory:
        root = Path(directory)
        for name in ('Incoming', 'Temp'):
            (root / name).mkdir()
        ports = free_ports(3, udp_indices=(2,), peer_index=1)
        write_amule_conf(root, ports[0], Port=ports[1], UDPPort=ports[2],
                         ConnectToKad=1, FilterLanIPs=0)
        proc, ec = start(binary, root, ports)
        try:
            assert ec.call(C['EC_OP_KAD_START'])[0] == C['EC_OP_NOOP']
            for index, length in enumerate((None, 0, 1, 18), start=2):
                address = f'127.0.0.{index}'
                with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp, \
                        socket.socket() as listener:
                    udp.bind((address, 0))
                    listener.bind((address, 0))
                    listener.listen(1)
                    if length is not None:
                        udp.sendto(b'\xc5\x95' + bytes(length), ('127.0.0.1', ports[2]))
                    callback(udp, ports[2], listener)
                    listener.settimeout(10 if length is None else 1)
                    try:
                        peer, _ = listener.accept()
                    except socket.timeout:
                        assert length is not None, 'positive control: a valid callback was ignored'
                    else:
                        peer.close()
                        assert length is None, f'{length}-byte callback bypassed the cooldown'
                assert proc.poll() is None, 'callback killed the daemon'
            print('PASS: valid callback connects; 0-, 1- and 18-byte requests consume the cooldown')
        except BaseException:
            print((root / 'logfile').read_text(errors='replace')[-6000:], file=sys.stderr)
            raise
        finally:
            ec.sock.close()
            stop_daemon(proc)


if __name__ == '__main__':
    retry_port_collisions(lambda: run(str(Path(sys.argv[1]).resolve())))
