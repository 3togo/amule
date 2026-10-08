# Menu icons and country flags

Prefer SVG for new artwork and use icons wherever they help users recognize an
action. Keep text labels, reuse the same symbol for the same action, and respect
native menu behavior. PNGs are compatibility fallbacks.

Menu artwork is from [Bootstrap Icons](https://github.com/twbs/icons), version
1.13.1, commit `ce0e49dd063243118a115f17ad1fe1fe7576d552` (MIT).
Country flags are from [flag-icons](https://github.com/lipis/flag-icons), version
7.5.0, commit `7aa5b2bdddd570ece62c812c0cb588ccdc099e2e` (MIT), the `flags/4x3`
directory. The manifest records both the annotated `v7.5.0` tag object and its
peeled commit; use the commit for raw-file URLs and source verification.
The individual copyright notices and complete licenses are retained
beside the original SVGs in `vendor/` and in the installed
[`docs/THIRDPARTY.md`](../../docs/THIRDPARTY.md).

`vendor/manifest.json` records upstream revisions, source directories and SHA-256
checksums of the unmodified originals. The editable originals must accompany
source distributions, along with the conversion script. No assets were copied
or traced from eMule or a screenshot.

The country selection retains every existing country code, adds missing ISO
country/territory flags and `xk`, and retains the existing `eu` flag. Extra
upstream symbols and non-country codes `cp`, `dg`, `ic`, `pc`, `un` and `xx` are
not imported. This is an artwork lookup, not a source of geolocation or country
names. An IP's estimated location does not identify a person's nationality.

`flags/an.png` and `flags/unknown.png` retain the existing FamFamFam public-domain
artwork by Mark James. They have no upstream SVG equivalent and remain raster
fallbacks. Other existing aMule icons are unchanged by this import.

## Regeneration

Normal builds use the checked-in assets; no downloads or SVG conversion tools
are needed. For artwork maintenance, install `picosvg==0.23.0` in a Python
virtual environment and install `rsvg-convert` (librsvg), then run:

```sh
python src/icons/regenerate_artwork.py
python src/icons/test_regenerate_artwork.py
python src/icons/embed_icons.py src/icons src/icons/icon_data.c
```

Conversion expands SVG use references but preserves supported shapes, strokes,
transforms and fill rules. Flags needing clipping and menu artwork use the full
picosvg compatibility pass; unsupported elements are passed to that converter
for validation rather than silently discarded. Converted flag paths use compact
relative coordinates when shorter, retaining the full pass's three-decimal
precision. Arc flags always have explicit separators for wx 3.2's NanoSVG.
The US/UM star markers are expanded
explicitly, with checks that reject changes to the expected marker layout.
The generated menu SVGs are 16×16;
flag SVGs and PNG fallbacks are 16×12. The rectangular 4:3 artwork is preserved,
not stretched to the former 16×11 flag size. PNG fallbacks are rendered from
the same normalized SVGs. Review representative flags with emblems, clipping,
stars and fine detail when updating the source set or converter.

Do not run every flag through full path conversion: expanding strokes and
transforms inflated the flag SVG payload from about 1.65 MB of original artwork
to 7.83 MB. Selective conversion and compact serialization keep all 251 flags
as vectors within a 3.5 MB maintenance-test budget before compression.
This budget concerns normalized SVG bytes, not the generated C file's hexadecimal
notation or compressed distribution size. Keep vendored originals and license
notices unchanged. When changing conversion, compare native wxWidgets renders
against the originals rendered with librsvg, including detailed flags and
transparent edges at 1x, 1.5x, 2x, 3x and 4x.

Menu SVGs use `#212529` as a replacement token. At popup creation,
`GetMenuBitmapBundle` substitutes the current system menu text colour outside
wxArtProvider's global cache and recolours PNG fallbacks while preserving alpha.
Previously cached neutral artwork cannot retain an old theme colour. On Cocoa,
menu images use a black alpha mask and are marked as AppKit templates after
insertion, letting the native menu supply appearance, selection and disabled
colours. Native menus retain responsibility for disabled states,
keyboard navigation, checkmarks and platform image preferences. Country flags
retain their original colours. The GUI caches vector bundles and a bounded set of completed bitmaps for each
logical size/backing scale. Missing codes share the unknown bitmap without
rescanning the embedded table. Windows icon/text cells draw HICON directly;
GTK and macOS continue using logical bitmap sizes.
The WebUI requests the embedded SVG through `/flags/{code}.svg`, with a PNG
fallback for legacy artwork and older daemons. Both routes are local and cached.

## Visual checks

With testing and a GUI enabled, build `IconArtworkTest` and run
`ctest --test-dir build -R IconArtworkTest --output-on-failure`. It checks all
bundled flags and menu symbols at 1×, 1.5× and 2×, simulated scale transitions,
unknown codes, repeated bitmap reuse, transparent legacy padding, menu command
IDs/mnemonics, disabled/check states, and repeated black/white theme-colour
changes in both SVG and forced PNG rendering. On Cocoa it also checks that the
native menu image is a template.
It skips when no GUI display is available, unless `AMULE_ICON_TEST_REQUIRE_GUI`
is set; CI sets it so missing native rendering is a failure. Linux GUI CI already
uses Xvfb, and now includes separate dark and high-contrast artwork tests.
The conversion/provenance/PNG-dimension/3.5 MB source-budget tests run in Icons CI. Set `AMULE_ICON_TEST_OUTPUT` to a
directory to export the native renders for inspection. On GTK, run all three theme checks with
`ctest --test-dir build -R 'IconArtwork.*Test' --output-on-failure` under Xvfb.

Check Downloads and category menus, shared-file actions, search actions and
server link copying at 100%, 150% and 200% display scaling. Check light, dark and
high-contrast themes, disabled commands, long translated labels, keyboard access
and priority checkmarks. Moving a list between monitors should resize flags
without blurring or shifting text. GTK may hide menu images according to the
desktop setting; the text and command behavior must remain complete.

## Embedded SVG compression

`embed_icons.py` stores each normalized SVG as a zlib stream (level 9), along
with its compressed and decoded lengths. PNGs remain unchanged. The shared C
helper `amule_decode_icon_svg` decodes into a caller-owned buffer and rejects
missing artwork, incorrect buffer lengths and damaged streams. It uses the
existing zlib dependency; Python's standard library performs build-time
compression. Builds without Python use the checked-in compressed table.

The GUI decodes on demand before NanoSVG parsing and menu colour replacement;
a decode failure follows the PNG fallback. Country flags retain their cached
vector bundles, avoiding repeated decompression while drawing list rows.
The API decodes before constructing its response, preserving ordinary SVG
bodies, content types, ETags and HEAD behavior. Internal storage compression
does not imply HTTP `Content-Encoding`.

For this artwork set, SVG payload decreases from 3,189,897 to 929,668 bytes
(70.9%), and generated C decreases from 20,363,109 to about 6,663,000 bytes.
These are source/storage measurements, not executable-size measurements.
`IconCompressionTest` verifies every decoded SVG byte against its source and
checks damaged streams and invalid buffer lengths without requiring a display.

The WebUI's actual `CountryCell` component has a Playwright check at device
pixel ratios 1 and 2, including all 252 ISO-code PNG entries (251 SVG twins),
legacy PNG fallback and hiding unavailable flags. Run
`node unittests/browser-tests/country-flags.cjs` with Playwright installed.
Set `AMULE_BROWSER_OUTPUT` to export screenshots. This fixture verifies the
component and source assets; live API bytes are verified separately by the
country-flag HTTP checks. Actual monitor changes, live OS appearance changes
and highlighted menu rows still require a Windows/macOS desktop session; do
not substitute simulated image sizes for those release checks.
