#!/usr/bin/env python3
"""Regenerate vendored menu icons and country flags; not needed for normal builds.

Requires picosvg==0.23.0 and rsvg-convert (librsvg). Source SVGs and licenses
live in vendor/. See ARTWORK.md for provenance and the pinned upstream revisions.
"""

from pathlib import Path
from copy import deepcopy
import re
import subprocess

from picosvg.svg import SVG
from picosvg.svg_types import SVGPath
from lxml import etree

ROOT = Path(__file__).resolve().parent


def expand_star_markers(svg):
    """Expand the simple marker-mid star layout in the pinned US/UM originals.

    NanoSVG and picosvg do not implement markers. Reject any other marker form
    so an upstream change cannot silently lose or incorrectly place the stars.
    """
    ns = {"s": "http://www.w3.org/2000/svg"}
    root = svg.svg_root
    for marker in list(root.findall("s:marker", ns)):
        assert set(marker.attrib) == {"id", "markerHeight", "markerWidth"}
        assert marker.get("markerHeight") == marker.get("markerWidth") == "30"
        uses = root.xpath('s:path[@marker-mid=$ref]', namespaces=ns,
                          ref=f'url(#{marker.get("id")})')
        assert len(uses) == 1
        path = uses[0]
        assert set(path.attrib) == {"fill", "marker-mid", "d"} and path.get("fill") == "none"
        commands = list(SVGPath(d=path.get("d")).absolute().as_cmd_seq())
        assert commands[0][0] == "M" and commands[-1][0] == "Z"
        assert len(commands[1:-1]) == 50 and all(c == "L" for c, _ in commands[1:-1])
        group = etree.Element("{http://www.w3.org/2000/svg}g")
        for _, (x, y) in commands[1:-1]:
            star = etree.SubElement(group, "{http://www.w3.org/2000/svg}g",
                                    transform=f"translate({x},{y})")
            for child in marker:
                star.append(deepcopy(child))
        root.replace(path, group)
        root.remove(marker)


def generate(source, destination, width, height, menu=False):
    text = source.read_text(encoding="utf-8")
    if menu:
        text = text.replace("currentColor", "#212529")
    # Some flag-icons stroke widths use points; picosvg expects SVG user units.
    text = re.sub(r'"([0-9.]+)pt"', lambda m: f'"{float(m[1]) * 4 / 3:g}"', text)
    svg = SVG.fromstring(text)
    expand_star_markers(svg)
    svg = svg.topicosvg()
    # Flatten use, clipping and even-odd fills into paths understood by NanoSVG.
    # Never silently drop unsupported artwork.
    svg.svg_root.set("width", str(width))
    svg.svg_root.set("height", str(height))
    svg.svg_root.attrib.pop("class", None)
    destination.write_text(svg.tostring() + "\n", encoding="utf-8")
    subprocess.run(["rsvg-convert", "--width", str(width), "--height", str(height),
                    "--output", str(destination.with_suffix(".png")), str(destination)], check=True)


def main():
    for source in sorted((ROOT / "vendor/bootstrap-icons").glob("*.svg")):
        generate(source, ROOT / ("menu_" + source.stem.replace("-", "_") + ".svg"), 16, 16, True)
    for source in sorted((ROOT / "vendor/flag-icons").glob("*.svg")):
        generate(source, ROOT / "flags" / source.name, 16, 12)


if __name__ == "__main__":
    main()
