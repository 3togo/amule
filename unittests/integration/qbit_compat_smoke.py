#!/usr/bin/env python3
"""Exercise compatibility routing against disposable, disconnected local daemons."""
import argparse
import hashlib
import http.cookiejar
import json
from pathlib import Path
import socket
import subprocess
import tempfile
import time
import urllib.error
import urllib.parse
import urllib.request


def port():
    with socket.socket() as sock:
        sock.bind(('127.0.0.1', 0))
        return sock.getsockname()[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--amuled', required=True)
    parser.add_argument('--amuleapi', required=True)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='amule-qbit-test-') as directory:
        root = Path(directory)
        incoming, temp = root / 'incoming', root / 'temp'
        incoming.mkdir(); temp.mkdir()
        ec, http_port, tcp = port(), port(), port()
        ec_password = 'disposable-ec-password'
        (root / 'amule.conf').write_text(f'''[eMule]
Nick=Compatibility test
Autoconnect=0
ConnectToKad=0
ConnectToED2K=0
NewVersionCheck=0
Port={tcp}
UDPPort=0
UDPDisable=1
TempDir={temp}
IncomingDir={incoming}
[ExternalConnect]
AcceptExternalConnections=1
ECAddress=127.0.0.1
ECPort={ec}
ECPassword={hashlib.md5(ec_password.encode()).hexdigest()}
[WebServer]
Enabled=0
[AmuleApi]
Enabled=0
''')
        (root / 'amuleapi.conf').write_text(f'''[Server]
BindAddress=127.0.0.1
Port={http_port}
QBitCompatibility=1
[EC]
Host=127.0.0.1
Port={ec}
Password={ec_password}
''')
        for role in ('admin', 'guest'):
            subprocess.run([args.amuleapi, '--config-dir', directory,
                            f'--set-{role}-pass=disposable-{role}'], check=True,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        logs = (root / 'process.log').open('w')
        processes = []
        try:
            processes.append(subprocess.Popen([args.amuled, '-c', directory, '-o'], stdout=logs, stderr=logs))
            deadline = time.monotonic() + 20
            while True:
                try:
                    with socket.create_connection(('127.0.0.1', ec), timeout=.2):
                        break
                except OSError:
                    if time.monotonic() > deadline or processes[0].poll() is not None:
                        raise AssertionError('amuled did not open the disposable EC listener')
                    time.sleep(.1)
            processes.append(subprocess.Popen([args.amuleapi, '--config-dir', directory], stdout=logs, stderr=logs))
            jar = http.cookiejar.CookieJar()
            opener = urllib.request.build_opener(urllib.request.ProxyHandler({}), urllib.request.HTTPCookieProcessor(jar))
            base = f'http://127.0.0.1:{http_port}/api/v2/'

            def request(route, data=None, expected=200, client=opener, headers=None):
                body = urllib.parse.urlencode(data).encode() if data is not None else None
                req = urllib.request.Request(base + route, data=body, headers=headers or {})
                try:
                    with client.open(req, timeout=5) as response:
                        code, result = response.status, response.read().decode()
                except urllib.error.HTTPError as response:
                    code, result = response.code, response.read().decode()
                assert code == expected, (route, code, result)
                return result

            deadline = time.monotonic() + 20
            while True:
                try:
                    assert request('app/webapiversion') == '2.11.0'
                    break
                except OSError:
                    if time.monotonic() > deadline: raise
                    time.sleep(.1)
            request('app/webapiversion', expected=403, headers={'Origin': 'https://untrusted.example'})
            request('app/version', expected=401)
            assert request('auth/login', {'username': 'admin', 'password': 'disposable-admin'}) == 'Ok.'
            cookies = list(jar)
            assert len(cookies) == 1 and cookies[0].name == 'SID' and cookies[0].path == '/api/v2'
            assert request('app/version') == 'v5.0.0-amule'
            # The first EC snapshot may still be in flight.
            deadline = time.monotonic() + 20
            while True:
                try:
                    assert json.loads(request('torrents/info')) == []
                    break
                except AssertionError:
                    if time.monotonic() > deadline: raise
                    time.sleep(.1)
            request('torrents/info?offset=-1', expected=400)
            request('torrents/info?hashes=%00', expected=400)
            request('torrents/createcategory', {'category': 'QBit test', 'savePath': str(incoming)})
            deadline = time.monotonic() + 10
            while 'QBit test' not in json.loads(request('torrents/categories')):
                assert time.monotonic() < deadline
                time.sleep(.1)
            h = '0123456789abcdef0123456789abcdef'
            link = f'ed2k://|file|qbit-test.bin|1024|{h}|/'
            request('torrents/add', {'urls': 'magnet:?xt=test'}, expected=400)
            request('torrents/add', {'urls': link, 'paused': 'true'}, expected=501)
            request('torrents/add', {'urls': link, 'category': 'QBit test'})
            deadline = time.monotonic() + 10
            while not json.loads(request('torrents/info')):
                assert time.monotonic() < deadline
                time.sleep(.1)
            files = json.loads(request('torrents/info'))
            assert files[0]['hash'] == h and files[0]['category'] == 'QBit test'
            assert json.loads(request(f'torrents/properties?hash={h}'))['total_size'] == 1024
            assert len(json.loads(request(f'torrents/files?hash={h}'))) == 1
            request('torrents/pause', {'hashes': h})
            request('torrents/resume', {'hashes': h})
            request('torrents/setcategory', {'hashes': h, 'category': ''})
            request('torrents/topPrio', {'hashes': h})
            request('torrents/delete', {'hashes': h, 'deleteFiles': 'false'}, expected=409)
            request('torrents/delete', {'hashes': h, 'deleteFiles': 'true'})
            request('auth/logout', {})
            request('app/version', expected=401)
            assert request('auth/login', {'username': 'guest', 'password': 'disposable-guest'}) == 'Ok.'
            request('torrents/add', {'urls': link}, expected=403)
            request('auth/logout', {})
            print('qBittorrent compatibility smoke test passed')
        except Exception:
            logs.flush()
            print((root / 'process.log').read_text()[-12000:])
            raise
        finally:
            for process in reversed(processes):
                process.terminate()
                try: process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill(); process.wait()
            logs.close()


if __name__ == '__main__':
    main()
