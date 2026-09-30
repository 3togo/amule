# Menu icons and country flags

Prefer SVG for new artwork and use icons wherever they help users recognize an
action. Keep text labels, reuse the same symbol for the same action, and respect
native menu behavior. PNGs are compatibility fallbacks.

Menu artwork is from [Bootstrap Icons](https://github.com/twbs/icons), version
1.13.1, commit `ce0e49dd063243118a115f17ad1fe1fe7576d552` (MIT).
Country flags are from [flag-icons](https://github.com/lipis/flag-icons), version
7.5.0, commit `50a8bff005239b0d2d661254094dedb9c75dbef3` (MIT), the `flags/4x3`
directory. The individual copyright notices and complete licenses are retained
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
python src/icons/embed_icons.py src/icons src/icons/icon_data.c
```

Conversion resolves SVG use elements, transforms, clipping, strokes and even-odd
fills into paths that wxWidgets' NanoSVG renderer understands. It fails on
unsupported content rather than dropping it. The US/UM star markers are expanded
explicitly, with checks that reject changes to the expected marker layout.
The generated menu SVGs are 16×16;
flag SVGs and PNG fallbacks are 16×12. The rectangular 4:3 artwork is preserved,
not stretched to the former 16×11 flag size. PNG fallbacks are rendered from
the same normalized SVGs. Review representative flags with emblems, clipping,
stars and fine detail when updating the source set or converter.

Menu SVGs use `#212529` as a replacement token: the art provider substitutes the
system menu text colour at menu creation and recolours PNG fallbacks while
preserving alpha. Native menus retain responsibility for disabled states,
keyboard navigation, checkmarks and platform image preferences. Country flags
retain their original colours. The GUI caches vector bundles and requests the
pixel size appropriate to the drawing window/DC rather than caching one 1× image.
The WebUI requests the embedded SVG through `/flags/{code}.svg`, with a PNG
fallback for legacy artwork and older daemons. Both routes are local and cached.

## Visual checks

With testing and a GUI enabled, build `IconArtworkTest` and run
`ctest --test-dir build -R IconArtworkTest --output-on-failure`. It checks all
bundled flags and menu symbols at 1×, 1.5× and 2×, monitor transitions, unknown
codes, menu command IDs/mnemonics, disabled/check states and system menu colour.
It skips when no GUI display is available. Set `AMULE_ICON_TEST_OUTPUT` to a
directory to export the native renders for inspection. On GTK, also run with
`GTK_THEME=Adwaita:dark` and `GTK_THEME=HighContrast`.

Check Downloads and category menus, shared-file actions, search actions and
server link copying at 100%, 150% and 200% display scaling. Check light, dark and
high-contrast themes, disabled commands, long translated labels, keyboard access
and priority checkmarks. Moving a list between monitors should resize flags
without blurring or shifting text. GTK may hide menu images according to the
desktop setting; the text and command behavior must remain complete.
