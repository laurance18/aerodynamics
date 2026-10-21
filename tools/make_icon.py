"""Generate resources/app.ico: a placeholder swirl icon (vortex streamlines).

Standard library only. Writes PNG-compressed images at 16, 32, 48 and 256 px into one .ico.
Usage: python tools/make_icon.py [output.ico]
"""

import math
import struct
import sys
import zlib
from pathlib import Path

SIZES = (16, 32, 48, 256)
SUPERSAMPLE = 4

BACKGROUND_TOP = (24, 44, 84)
BACKGROUND_BOTTOM = (10, 18, 38)
LINE_INNER = (255, 190, 80)
LINE_OUTER = (80, 200, 255)


def lerp(a, b, t):
    return tuple(x + (y - x) * t for x, y in zip(a, b))


def sample(u, v):
    """Colour (r, g, b, a) at normalised coords u, v in [0, 1]."""
    # Rounded-square mask.
    corner = 0.2
    dx = max(abs(u - 0.5) - (0.5 - corner), 0.0)
    dy = max(abs(v - 0.5) - (0.5 - corner), 0.0)
    if dx * dx + dy * dy > corner * corner:
        return (0.0, 0.0, 0.0, 0.0)

    color = lerp(BACKGROUND_TOP, BACKGROUND_BOTTOM, v)
    x, y = u - 0.5, v - 0.5
    r = math.hypot(x, y)
    if r < 0.43:
        # Logarithmic-spiral streamlines: theta - k ln r = const.
        arms = 4
        theta = math.atan2(y, x)
        phase = (theta - 2.2 * math.log(max(r, 1e-4))) * arms / (2.0 * math.pi)
        dist = abs(phase - round(phase))
        width = 0.10 + 0.12 * (r / 0.43)
        if dist < width and r > 0.05:
            fade = 1.0 - (r / 0.43) ** 3
            line = lerp(LINE_INNER, LINE_OUTER, min(r / 0.35, 1.0))
            color = lerp(color, line, fade)
        if r < 0.06:
            color = LINE_INNER
    return (*color, 255.0)


def render(size):
    rows = []
    n = SUPERSAMPLE
    for py in range(size):
        row = bytearray([0])  # PNG filter type: none
        for px in range(size):
            acc = [0.0, 0.0, 0.0, 0.0]
            for sy in range(n):
                for sx in range(n):
                    c = sample((px + (sx + 0.5) / n) / size, (py + (sy + 0.5) / n) / size)
                    acc[0] += c[0] * c[3]
                    acc[1] += c[1] * c[3]
                    acc[2] += c[2] * c[3]
                    acc[3] += c[3]
            a = acc[3] / (n * n)
            if acc[3] > 0:
                rgb = [acc[i] / acc[3] for i in range(3)]
            else:
                rgb = [0, 0, 0]
            row += bytes(int(round(min(max(ch, 0), 255))) for ch in (*rgb, a))
        rows.append(bytes(row))
    return b"".join(rows)


def png_bytes(size):
    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data))

    ihdr = struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0)  # 8-bit RGBA
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr)
            + chunk(b"IDAT", zlib.compress(render(size), 9)) + chunk(b"IEND", b""))


def main():
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).parent.parent / "resources" / "app.ico"
    images = [png_bytes(s) for s in SIZES]
    header = struct.pack("<HHH", 0, 1, len(images))
    offset = 6 + 16 * len(images)
    entries = b""
    for size, data in zip(SIZES, images):
        dim = 0 if size >= 256 else size
        entries += struct.pack("<BBBBHHII", dim, dim, 0, 0, 1, 32, len(data), offset)
        offset += len(data)
    out.write_bytes(header + entries + b"".join(images))
    print(f"wrote {out}")


if __name__ == "__main__":
    main()
