// Run the production magnet-link builder without a browser or a daemon.
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import { test } from 'node:test';

// Import the production ES module without changing the repository's package type.
const source = await readFile(new URL('../../src/webapi/static/js/links.js', import.meta.url));
const { magnetLink } = await import(`data:text/javascript;base64,${source.toString('base64')}`);
const file = {
  name: 'example file.iso',
  hash: 'ABCDEF0123456789ABCDEF0123456789',
  size_bytes: 4294967297,
};
const base = 'magnet:?dn=example%20file.iso' +
  '&xt=urn:ed2k:abcdef0123456789abcdef0123456789' +
  '&xt=urn:ed2khash:abcdef0123456789abcdef0123456789&xl=4294967297';

test('includes the API AICH master hash as an additional exact topic', () => {
  const aich = 'QVVWMK4S7ZJU63ASZLPFYU4A5WOAAAAA';
  const link = magnetLink({ ...file, aich_hash: aich });
  assert.equal(link, base + '&xt=urn:aich:' + aich);
  assert.deepEqual(new URL(link).searchParams.getAll('xt'), [
    'urn:ed2k:' + file.hash.toLowerCase(),
    'urn:ed2khash:' + file.hash.toLowerCase(),
    'urn:aich:' + aich,
  ]);
});

test('preserves links for files without an AICH hash', () => {
  for (const aich_hash of [undefined, null, '']) {
    assert.equal(magnetLink({ ...file, aich_hash }), base);
  }
});

test('preserves desktop filename cleanup and zero-size handling', () => {
  assert.equal(magnetLink({ ...file, name: '/测试\u0001 file.iso', size_bytes: 0 }),
    base.replace('example%20file.iso', '测试%20file.iso').replace('4294967297', '0'));
});

test('handles missing optional metadata', () => {
  assert.equal(magnetLink({}), 'magnet:?dn=&xt=urn:ed2k:&xt=urn:ed2khash:&xl=0');
});
