# Windows artwork support

The GUI already uses a PerMonitorV2/PerMonitor manifest and Common Controls v6.
The native artwork test embeds the same manifest. Country flags retain vector
bundles and render at the calling window's drawing size and backing scale;
menu icons use bundles and preserve native labels, checkmarks and disabled
states. Zlib-compressed SVG storage works independently of the Windows SVG
renderer and uses the application's existing zlib dependency.

## Recommended next work

1. **Verify real monitor transitions.** Keep the current headless compression
   test and native rendering test in Windows Debug and Release CI. Add a
   window-based test or manual release check that moves both GUIs between
   100%, 150%, 200% and 300% monitors, opens every affected context menu, and
   checks flag placement and text alignment. The current test simulates image
   sizes; it cannot establish that native menus and list geometry respond to
   actual `WM_DPICHANGED` messages. Check custom list row heights and padding
   for updates after `wxEVT_DPI_CHANGED`, and refresh affected controls if
   they retain dimensions from the old monitor. Avoid multiplying `FromDIP`
   results by the DPI scale again on wxMSW.

2. **Verify live theme changes.** The fix now colours each new popup outside
   wxArtProvider's cached bundles, including PNG fallbacks. Automated tests
   switch black/white/black while neutral artwork stays cached. This avoids
   relying on Push/Pop, which leaves the bundle cache intact in wxWidgets 3.2.
   Run the actual application through light/dark and Windows Contrast theme
   changes without restarting, including a highlighted and a disabled menu
   row. Flags retain their original colours.

3. **Test the delivered package.** Launch the portable ZIP and installer on a
   clean supported Windows machine, rather than only from an MSYS2 build
   shell. Check wxWidgets and zlib DLL discovery, the executable's embedded
   manifest, SVG rendering, PNG fallback, and `/flags/{code}.svg`. Test both
   monolithic and remote GUIs. This catches packaging and loader failures
   that the source-tree tests do not cover.

4. **Make toolchain support explicit.** The CI baseline is MSYS2 CLANG64 with
   wxWidgets 3.3; record that tested combination in release instructions.
   Treat ARM64 as a separate build/test matrix before claiming parity.
   If supporting native MSVC is desired, add a pinned dependency setup and
   CMake preset plus a native build job, rather than inferring support from
   MinGW/Clang results. Use imported `ZLIB::ZLIB` targets so include paths and
   transitive linkage follow the selected toolchain.

## Validation commands

Configure with `BUILD_TESTING=ON` and at least one GUI enabled, then run:

```sh
cmake --build build --target IconCompressionTest IconArtworkTest
ctest --test-dir build -R 'Icon(Compression|Artwork).*Test' --output-on-failure
```

`IconCompressionTest` needs no display. `IconArtworkTest` needs a native GUI
session and may skip if initialization fails; a skipped result does not count
as Windows visual verification. Export renders with `AMULE_ICON_TEST_OUTPUT`
for inspection, and run `unittests/curl-tests/amuleapi/32-country-flags.sh`
against the built API server to verify SVG/PNG HTTP behavior.
