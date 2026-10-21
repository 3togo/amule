#!/usr/bin/env python3
# Copyright (c) 2026 aMule Team
# SPDX-License-Identifier: GPL-2.0-or-later
"""Build the real Networks page in an offline GUI; layout assertions are fatal."""
import os
from pathlib import Path
import subprocess
import struct
import sys
import tempfile
import time

from AllSearchIntegrationTest import connect_daemon, free_ports, retry_port_collisions, stop_daemon
from DaemonConfig import write_amule_conf


def run(gui, daemon, root):
    (root / 'Incoming').mkdir()
    (root / 'Temp').mkdir()
    ports = free_ports(2, peer_index=1)
    (root / 'server.met').write_bytes(b'\xe0' + struct.pack('<I', 1) +
                                      bytes([203, 0, 113, 9]) + struct.pack('<HI', 4661, 0))
    write_amule_conf(root, ports[0], Port=ports[1], UDPEnable=0,
                     ConnectToED2K=1, ConfirmExit=0, StartupMinimized=1)
    env = dict(os.environ, HOME=str(root), XDG_CONFIG_HOME=str(root / 'xdg'),
               LC_ALL='C.UTF-8', LANG='C.UTF-8')
    env.pop('LD_PRELOAD', None)
    # Seed version notices and the user hash with the same build, so a fresh-profile
    # welcome dialog cannot hide a failure while constructing the real main window.
    with (root / 'seed-console.log').open('w') as console:
        seed = subprocess.Popen([daemon, '-c', str(root), '--disable-fatal'],
                                env=env, stdout=console, stderr=subprocess.STDOUT)
        try:
            ec = connect_daemon(seed, ports[0], root, ports[1], server_udp=True)
            ec.sock.close()
        finally:
            stop_daemon(seed)
    console_file = root / 'gui-console.log'
    with console_file.open('w') as console:
        proc = subprocess.Popen([gui, '-c', str(root), '--disable-fatal', '-o'],
                                env=env, stdout=console, stderr=subprocess.STDOUT)
        try:
            deadline = time.monotonic() + 25
            ready_at = None
            while True:
                text = console_file.read_text(errors='replace')
                assert proc.poll() is None, text
                assert 'Assertion failed:' not in text, text
                # Shared-file scanning follows construction of CamuleDlg/CServerWnd.
                if 'Found 0 known shared files' in text:
                    ready_at = ready_at or time.monotonic()
                    if time.monotonic() - ready_at >= 1:
                        break
                assert time.monotonic() < deadline, text
                time.sleep(0.05)
        finally:
            if proc.poll() is None:
                if sys.exc_info()[0] is None:
                    stop_daemon(proc)
                else:
                    proc.terminate()
                    proc.wait(timeout=10)


def main():
    if not (os.environ.get('DISPLAY') or os.environ.get('WAYLAND_DISPLAY') or
            os.environ.get('GDK_BACKEND') == 'broadway'):
        print('SKIP: GUI startup needs a display (use xvfb-run).')
        return 77
    gui, daemon = (str(Path(arg).resolve()) for arg in sys.argv[1:3])

    def attempt():
        with tempfile.TemporaryDirectory(prefix='amule-server-gui-') as temporary:
            run(gui, daemon, Path(temporary))

    retry_port_collisions(attempt)
    print('Real Networks-page startup passed without layout assertions')
    return 0


if __name__ == '__main__':
    sys.exit(main())
