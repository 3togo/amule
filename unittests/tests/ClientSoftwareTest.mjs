// Run the production formatter without a browser or experimental VM modules.
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import { test } from 'node:test';

const source = await readFile(new URL('../../src/webapi/static/js/client-software.js', import.meta.url));
const { clientSoftwareLabel } = await import(`data:text/javascript;base64,${source.toString('base64')}`);
const english = JSON.parse(await readFile(new URL('../../src/webapi/static/i18n/en.json', import.meta.url), 'utf8'));
const cases = [
  [{ software: 'emule', software_version: 'v0.70b', client_mod_name: 'AI v1.6' }, 'eMule (AI v1.6) v0.70b'],
  [{ software: 'emule', software_version: 'v0.70b', client_mod_name: 'eMule Qt 1.0' }, 'eMule (eMule Qt 1.0) v0.70b'],
  [{ software: 'emule', software_version: 'v0.70b', client_mod_name: null }, 'eMule v0.70b'],
  [{ software: 'amule', software_version: 'v2.3.3', client_mod_name: 'aMule' }, 'aMule v2.3.3'],
  [{ software: 'amule', software_version: 'v2.3.3', client_mod_name: 'aMule SVN' }, 'aMule SVN v2.3.3'],
  [{ software: 'old_emule', software_version: 'v0.49a', client_mod_name: 'old-mod' }, 'Old eMule (old-mod) v0.49a'],
  [{}, '—'],
];

for (const [row, expected] of cases) {
  test(`formats ${expected}`, () => {
    const familyName = row.software ? english['downloads_peer_soft_' + row.software] : '';
    assert.equal(clientSoftwareLabel(row, familyName), expected);
  });
}

test('uses the caller-provided translated family name', () => {
  assert.equal(clientSoftwareLabel({ software: 'unknown', software_version: 'Unknown' }, 'Inconnu'), 'Inconnu');
});
