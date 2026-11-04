"""Bake original Comic Sans MS 24-pixel glyphs into a portable runtime atlas.

This is build-time asset preparation only. It uses an offscreen DIB and the
original CreateFontA parameters, never a window or desktop capture.
"""

import ctypes
import hashlib
import json
import os
import shutil
import struct
from ctypes import wintypes
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
NAME = "comic-sans-ms-24.bhf"
WIDTH, HEIGHT = 1024, 768
CELL_W, CELL_H, ORIGIN_X, ORIGIN_Y = 64, 48, 16, 8


class BitmapHeader(ctypes.Structure):
    _fields_ = [
        ("size", wintypes.DWORD),
        ("width", wintypes.LONG),
        ("height", wintypes.LONG),
        ("planes", wintypes.WORD),
        ("bits", wintypes.WORD),
        ("compression", wintypes.DWORD),
        ("image_size", wintypes.DWORD),
        ("xppm", wintypes.LONG),
        ("yppm", wintypes.LONG),
        ("colors", wintypes.DWORD),
        ("important", wintypes.DWORD),
    ]


def api():
    gdi = ctypes.WinDLL("gdi32", use_last_error=True)
    definitions = {
        "CreateCompatibleDC": ([wintypes.HDC], wintypes.HDC),
        "DeleteDC": ([wintypes.HDC], wintypes.BOOL),
        "CreateDIBSection": (
            [
                wintypes.HDC,
                ctypes.c_void_p,
                wintypes.UINT,
                ctypes.POINTER(ctypes.c_void_p),
                wintypes.HANDLE,
                wintypes.DWORD,
            ],
            wintypes.HBITMAP,
        ),
        "CreateFontA": (
            [ctypes.c_int] * 5 + [wintypes.DWORD] * 8 + [ctypes.c_char_p],
            wintypes.HFONT,
        ),
        "SelectObject": ([wintypes.HDC, wintypes.HGDIOBJ], wintypes.HGDIOBJ),
        "DeleteObject": ([wintypes.HGDIOBJ], wintypes.BOOL),
        "SetBkMode": ([wintypes.HDC, ctypes.c_int], ctypes.c_int),
        "SetTextColor": ([wintypes.HDC, wintypes.COLORREF], wintypes.COLORREF),
        "TextOutA": (
            [wintypes.HDC, ctypes.c_int, ctypes.c_int, ctypes.c_char_p, ctypes.c_int],
            wintypes.BOOL,
        ),
        "GetTextExtentPoint32A": (
            [
                wintypes.HDC,
                ctypes.c_char_p,
                ctypes.c_int,
                ctypes.POINTER(wintypes.SIZE),
            ],
            wintypes.BOOL,
        ),
        "GetTextFaceA": ([wintypes.HDC, ctypes.c_int, ctypes.c_char_p], ctypes.c_int),
        "GdiFlush": ([], wintypes.BOOL),
    }
    for name, (arguments, result) in definitions.items():
        method = getattr(gdi, name)
        method.argtypes, method.restype = arguments, result
    return gdi


def offscreen(width, height):
    gdi = api()
    dc = gdi.CreateCompatibleDC(None)
    header = BitmapHeader(
        ctypes.sizeof(BitmapHeader), width, -height, 1, 32, 0, 0, 0, 0, 0, 0
    )
    bits = ctypes.c_void_p()
    bitmap = gdi.CreateDIBSection(
        dc, ctypes.byref(header), 0, ctypes.byref(bits), None, 0
    )
    font = gdi.CreateFontA(24, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 3, 2, b"Comic Sans MS")
    if not dc or not bitmap or not font or not bits.value:
        raise OSError("Unable to prepare the original Comic Sans font")
    old_bitmap = gdi.SelectObject(dc, bitmap)
    old_font = gdi.SelectObject(dc, font)
    face = ctypes.create_string_buffer(128)
    gdi.GetTextFaceA(dc, len(face), face)
    if face.value != b"Comic Sans MS":
        raise RuntimeError(f"Comic Sans MS is required; GDI selected {face.value!r}")
    gdi.SetBkMode(dc, 1)
    gdi.SetTextColor(dc, 0xFFFFFF)
    memory = (ctypes.c_ubyte * (width * height * 4)).from_address(bits.value)
    ctypes.memset(bits, 0, len(memory))
    return gdi, dc, bitmap, font, old_bitmap, old_font, memory


def close(state):
    gdi, dc, bitmap, font, old_bitmap, old_font, _ = state
    gdi.SelectObject(dc, old_font)
    gdi.SelectObject(dc, old_bitmap)
    gdi.DeleteObject(font)
    gdi.DeleteObject(bitmap)
    gdi.DeleteDC(dc)


def bake():
    state = offscreen(WIDTH, HEIGHT)
    gdi, dc, *_, memory = state
    metrics = []
    try:
        for code in range(256):
            text = bytes([code])
            size = wintypes.SIZE()
            if not gdi.GetTextExtentPoint32A(dc, text, 1, ctypes.byref(size)):
                raise OSError(f"Cannot measure character {code}")
            x, y = code % 16 * CELL_W + ORIGIN_X, code // 16 * CELL_H + ORIGIN_Y
            if not gdi.TextOutA(dc, x, y, text, 1):
                raise OSError(f"Cannot rasterize character {code}")
            metrics.append((x, y, size.cx))
        gdi.GdiFlush()
        raw = bytes(memory)
        mask = raw[0::4]
        if set(mask) - {0, 255}:
            raise RuntimeError(
                "Original NONANTIALIASED font unexpectedly produced coverage levels"
            )
        records = []
        for code, (origin_x, origin_y, advance) in enumerate(metrics):
            cell_x, cell_y = code % 16 * CELL_W, code // 16 * CELL_H
            occupied = [
                (x, y)
                for y in range(cell_y, cell_y + CELL_H)
                for x in range(cell_x, cell_x + CELL_W)
                if mask[y * WIDTH + x]
            ]
            if occupied:
                left, right = (
                    min(x for x, _ in occupied),
                    max(x for x, _ in occupied) + 1,
                )
                top, bottom = (
                    min(y for _, y in occupied),
                    max(y for _, y in occupied) + 1,
                )
                if (
                    left <= cell_x
                    or right >= cell_x + CELL_W
                    or top <= cell_y
                    or bottom >= cell_y + CELL_H
                ):
                    raise RuntimeError(f"Glyph {code} exceeds its bake cell")
                records.append(
                    struct.pack(
                        "<4H4h",
                        left,
                        top,
                        right - left,
                        bottom - top,
                        left - origin_x,
                        top - origin_y,
                        advance,
                        0,
                    )
                )
            else:
                records.append(struct.pack("<4H4h", 0, 0, 0, 0, 0, 0, advance, 0))
        return (
            struct.pack("<8s4I", b"BHCFONT1", WIDTH, HEIGHT, 24, 256)
            + b"".join(records)
            + mask
        )
    finally:
        close(state)


def prepare(out):
    target = Path(out) / "fonts" / NAME
    target.parent.mkdir(parents=True, exist_ok=True)
    supplied = ROOT / "assets/fonts" / NAME
    if supplied.is_file():
        shutil.copy2(supplied, target)
        return target
    if os.name != "nt":
        raise RuntimeError(f"Provide the baked original font at {supplied}")
    source = Path(os.environ.get("WINDIR", "C:/Windows")) / "Fonts/comic.ttf"
    identity = hashlib.sha256(
        source.read_bytes() + Path(__file__).read_bytes()
    ).hexdigest()
    cache = ROOT / "build-cache/fonts"
    cache.mkdir(parents=True, exist_ok=True)
    cached, stamp = cache / NAME, cache / "comic-sans-ms-24.json"
    metadata = json.loads(stamp.read_text()) if stamp.exists() else {}
    if not cached.exists() or metadata.get("identity") != identity:
        cached.write_bytes(bake())
        metadata = {
            "identity": identity,
            "font_source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
            "font": "Comic Sans MS",
            "height": 24,
            "weight": 400,
            "quality": 3,
            "atlas_sha256": hashlib.sha256(cached.read_bytes()).hexdigest(),
        }
        stamp.write_text(json.dumps(metadata, indent=2) + "\n")
    shutil.copy2(cached, target)
    return target


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(prepare(args.output))
