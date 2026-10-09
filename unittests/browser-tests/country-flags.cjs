// Run with Node and Playwright installed: node unittests/browser-tests/country-flags.cjs
// Set AMULE_BROWSER_OUTPUT to retain 1x/2x/3x screenshots for visual inspection.
// Set AMULE_BROWSER_EXECUTABLE to use an existing Chromium installation.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const http = require('node:http');
const { chromium } = require('playwright');

const root = path.resolve(__dirname, '../..');
const staticRoot = path.join(root, 'src/webapi/static');
const flagsRoot = path.join(root, 'src/icons/flags');
const codes = fs.readdirSync(flagsRoot).filter(name => /^[a-z]{2}\.png$/.test(name))
  .map(name => name.slice(0, 2)).sort();
const importMap = fs.readFileSync(path.join(staticRoot, 'index.html'), 'utf8')
  .match(/<script type="importmap">([\s\S]*?)<\/script>/)[1];
const html = `<!doctype html><html lang="en"><head><meta charset="utf-8">
<link rel="stylesheet" href="/css/app.css"><script type="importmap">${importMap}</script>
<style>body{padding:24px}#flags{display:grid;grid-template-columns:repeat(12,1fr);gap:10px}
.country-cell{min-height:24px}</style></head><body><h1>WebUI country flags</h1><div id="flags"></div>
<script type="module">
import { CountryCell } from '/js/components.js';
import { html, render } from '/js/dom.js';
const codes = ${JSON.stringify([...codes, 'zz'])};
render(html\`\${codes.map(code => html\`<div data-code=\${code}><\${CountryCell} code=\${code}/></div>\`)}\`, document.querySelector('#flags'));
window.flagsReady = true;
</script></body></html>`;
const server = http.createServer((req, res) => {
  const url = new URL(req.url, 'http://localhost');
  if (url.pathname === '/') {
    res.setHeader('Content-Type', 'text/html');
    res.end(html);
    return;
  }
  const isFlag = url.pathname.startsWith('/flags/');
  const base = isFlag ? flagsRoot : staticRoot;
  if (isFlag && /@[23]x\.png$/.test(url.pathname) && req.headers.cookie === 'legacy=1') {
    res.writeHead(404); res.end(); return;
  }
  const file = path.resolve(base, '.' + (isFlag ? url.pathname.slice(6) : url.pathname));
  if (!file.startsWith(base + path.sep) || !fs.existsSync(file) || !fs.statSync(file).isFile()) {
    res.writeHead(404); res.end(); return;
  }
  const types = { '.svg': 'image/svg+xml', '.png': 'image/png', '.js': 'text/javascript', '.css': 'text/css', '.json': 'application/json' };
  res.setHeader('Content-Type', types[path.extname(file)] || 'application/octet-stream');
  res.end(fs.readFileSync(file));
});
(async () => {
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  const browser = await chromium.launch({ executablePath: process.env.AMULE_BROWSER_EXECUTABLE || undefined });
  try {
    for (const legacy of [false, true]) {
      for (const scale of [1, 2, 3]) {
        const context = await browser.newContext({ viewport: { width: 1200, height: 850 }, deviceScaleFactor: scale });
        if (legacy) await context.addCookies([{ name: 'legacy', value: '1', url: `http://127.0.0.1:${server.address().port}` }]);
        const page = await context.newPage();
        const errors = [];
        page.on('pageerror', error => errors.push(error.message));
        await page.goto(`http://127.0.0.1:${server.address().port}/`);
        await page.waitForFunction(() => window.flagsReady);
        await page.waitForFunction(() => [...document.querySelectorAll('img.flag')].every(image => image.complete));
        await page.waitForFunction(({ legacy, scale }) => [...document.querySelectorAll('[data-code]')].every(cell => {
          const image = cell.querySelector('img');
          if (cell.dataset.code === 'zz') return image.style.visibility === 'hidden';
          const density = legacy ? 1 : scale;
          return image.complete && image.naturalWidth > 0 && image.currentSrc.endsWith(
            `${cell.dataset.code}${density === 1 ? '' : '@' + density + 'x'}.png`);
        }), { legacy, scale });
        const images = await page.locator('[data-code]').evaluateAll(cells => cells.map(cell => {
          const image = cell.querySelector('img');
          const rect = image.getBoundingClientRect();
          return { code: cell.dataset.code, src: image.currentSrc, naturalWidth: image.naturalWidth, loaded: image.naturalWidth > 0,
            width: rect.width, height: rect.height, hidden: image.style.visibility === 'hidden' };
        }));
        assert.equal(await page.evaluate(() => devicePixelRatio), scale);
        assert.deepEqual(errors, []);
        assert.equal(images.length, codes.length + 1);
        for (const image of images) {
          assert.equal(image.width, 16, image.code);
          assert.equal(image.height, 12, image.code);
          if (image.code === 'zz') {
            assert.equal(image.hidden, true);
          } else {
            assert.equal(image.loaded, true, image.code);
            assert.equal(image.hidden, false, image.code);
            const density = legacy ? 1 : scale;
            assert.ok(image.src.endsWith(`${image.code}${density === 1 ? '' : '@' + density + 'x'}.png`), image.code);
          }
        }
        if (process.env.AMULE_BROWSER_OUTPUT) {
          fs.mkdirSync(process.env.AMULE_BROWSER_OUTPUT, { recursive: true });
          await page.screenshot({ path: path.join(process.env.AMULE_BROWSER_OUTPUT, `flags-${legacy ? "legacy-" : ""}${scale}x.png`), fullPage: true });
        }
        console.log(`PASS: ${codes.length} WebUI flags (${legacy ? "1x-only API" : "density-aware API"}) at ${scale}x, srcset selection, 1x fallback and missing flag`);
        await context.close();
      }
    }
  } finally {
    await browser.close();
    server.close();
  }
})().catch(error => { console.error(error); server.close(); process.exitCode = 1; });
