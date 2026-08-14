#!/usr/bin/env python3
"""Generate the 10x10 1-bit app icon (a garage with a panelled door).

Flipper app icons are tiny monochrome PNGs. Rather than committing an opaque
binary blob, we keep the pixel art readable here and regenerate on demand:

    python3 tools/make_icon.py

Only the Python standard library is used (zlib + struct), so this runs anywhere.
"""

import struct
import zlib
from pathlib import Path

# '#' = black (lit pixel), '.' = white. Roof on top, panelled door below.
ART = [
    "..######..",
    ".########.",
    "##########",
    "#........#",
    "#.######.#",
    "#.#....#.#",
    "#.######.#",
    "#.#....#.#",
    "#.######.#",
    "##########",
]

WIDTH = len(ART[0])
HEIGHT = len(ART)


def _chunk(tag: bytes, data: bytes) -> bytes:
    return (
        struct.pack(">I", len(data))
        + tag
        + data
        + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
    )


def build_png() -> bytes:
    for row in ART:
        assert len(row) == WIDTH, "all icon rows must be the same width"

    # 8-bit greyscale, one filter byte (0 = None) per scanline.
    raw = b"".join(
        b"\x00" + bytes(0x00 if px == "#" else 0xFF for px in row) for row in ART
    )

    return b"".join(
        [
            b"\x89PNG\r\n\x1a\n",
            _chunk(b"IHDR", struct.pack(">IIBBBBB", WIDTH, HEIGHT, 8, 0, 0, 0, 0)),
            _chunk(b"IDAT", zlib.compress(raw, 9)),
            _chunk(b"IEND", b""),
        ]
    )


def main() -> None:
    out = Path(__file__).resolve().parent.parent / "icons" / "garagemate_10px.png"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(build_png())
    print(f"wrote {out} ({WIDTH}x{HEIGHT})")


if __name__ == "__main__":
    main()
