"""Build Crossroads, an isolated BHunter deathmatch test arena.

Run after building the game: python tools/build_test_arena.py
The generated runtime is build/test-arena; play.cmd starts a local host.
Only loose assets in that directory override the original game archive.
"""

import argparse
import json
import shutil
import struct
from itertools import pairwise
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def pack(fmt, *values):
    return struct.pack("<" + fmt, *values)


class Model:
    """Axis-aligned convex surfaces with coplanar polygon lists."""

    def __init__(self):
        self.materials = []
        self.groups = {}

    def rectangle(self, axis, plane, a, b, c, d, normal, texture, tile=16):
        if texture not in self.materials:
            self.materials.append(texture)
        u, v = [i for i in range(3) if i != axis]
        # Split textured planes into tiles rather than relying on UV wrapping.
        us = (
            [a] + [a + tile * i for i in range(1, int((b - a - 1e-5) / tile) + 1)] + [b]
        )
        vs = (
            [c] + [c + tile * i for i in range(1, int((d - c - 1e-5) / tile) + 1)] + [d]
        )
        for u0, u1 in pairwise(us):
            for v0, v1 in pairwise(vs):
                points = []
                for x, y in ((u0, v0), (u1, v0), (u1, v1), (u0, v1)):
                    p = [0.0, 0.0, 0.0]
                    p[axis], p[u], p[v] = plane, x, y
                    points.append(p)
                sign = -1 if axis == 1 else 1
                uv = [
                    (0.0, 0.0),
                    ((u1 - u0) / tile, 0.0),
                    ((u1 - u0) / tile, (v1 - v0) / tile),
                    (0.0, (v1 - v0) / tile),
                ]
                # X-facing walls span Y/Z: map height (Y) to texture V,
                # matching the upright window rows on Z-facing walls.
                if axis == 0:
                    uv = [(tv, tu) for tu, tv in uv]
                # These retail facade images are stored a quarter-turn sideways.
                if axis != 1 and texture in ("DT_BIG01.PCX", "F_GLAS01.PCX"):
                    uv = [(tv, 1.0 - tu) for tu, tv in uv]
                if sign != normal:
                    points.reverse()
                    uv.reverse()
                self.groups.setdefault((axis, plane, normal), []).append(
                    (points, uv, self.materials.index(texture))
                )

    def write(self, path, inward):
        vertices, edges, faces, leaves, nodes = [], [], [], [], []
        points = [
            p for group in self.groups.values() for face, _, _ in group for p in face
        ]
        low = [min(p[i] for p in points) for i in range(3)]
        high = [max(p[i] for p in points) for i in range(3)]
        groups = list(self.groups.items())
        for node_index, ((axis, plane, sign), polygons) in enumerate(groups):
            leaf_start = len(leaves)
            for polygon_index, (poly, uv, material) in enumerate(polygons):
                base = len(vertices)
                for j, (p, tex) in enumerate(zip(poly, uv)):
                    vertices.append(pack("3f", *p) + bytes(48))
                    edges.append(
                        pack("iiff", base + j, base + j + 1 if j < 3 else -1, *tex)
                    )
                faces.append(pack("iii", material, base, 0))
                leaves.append(
                    pack(
                        "ii",
                        len(faces) - 1,
                        len(leaves) + 1 if polygon_index + 1 < len(polygons) else -1,
                    )
                )
            normal = [0.0, 0.0, 0.0]
            normal[axis] = sign
            child = node_index + 1 if node_index + 1 < len(groups) else -1
            nodes.append(
                pack(
                    "10f4i",
                    *low,
                    *high,
                    *normal,
                    -sign * plane,
                    leaf_start,
                    -1,
                    child if inward else -1,
                    -1 if inward else child,
                )
            )
        materials = [
            bytes((50, 50, 50, 180, 180, 180, 229, 229, 229))
            + name.encode("ascii").ljust(23, b"\0")
            for name in self.materials
        ]
        path.write_bytes(
            b"".join(
                pack("I", len(rows)) + b"".join(rows)
                for rows in (materials, vertices, edges, faces, nodes, leaves)
            )
        )
        return {
            "model": path.name,
            "minimum": low,
            "maximum": high,
            "faces": len(faces),
        }


def solid(path, width, depth, height, wall):
    m = Model()
    lo, hi = [-width / 2, -height, -depth / 2], [width / 2, 0, depth / 2]
    for axis in range(3):
        u, v = [i for i in range(3) if i != axis]
        for side in range(2):
            m.rectangle(
                axis,
                (lo, hi)[side][axis],
                lo[u],
                hi[u],
                lo[v],
                hi[v],
                -1 if side == 0 else 1,
                "P_ROOF2.PCX" if axis == 1 else wall,
                16,
            )
    return m.write(path, inward=False)


def street(x, z):
    return (
        any(abs(abs(q) - 40) < 10 or abs(abs(q) - 110) < 8 for q in (x, z))
        or (abs(x) < 10 and abs(z) < 40)
        or (abs(z) < 10 and abs(x) < 40)
    )


def road_texture(folder, x, z, cache):
    """Palette PCX tiles with dashed centre lines and solid street-edge paint."""
    pixels = bytearray()
    for py in range(64):
        for px in range(64):
            wx, wz = x + (px + 0.5) * 10 / 64, z + (py + 0.5) * 10 / 64
            vx = min(abs(wx - c) for c in (-110, -40, 40, 110))
            hz = min(abs(wz - c) for c in (-110, -40, 40, 110))
            vertical = vx < 10
            horizontal = hz < 10
            value = 32 + (px * 13 + py * 17 + px * py * 3) % 9
            if vertical and not horizontal:
                if vx < 0.22 and int(wz // 5) % 2 == 0:
                    value = 240
                if 8.2 < vx < 8.45:
                    value = 241
            if horizontal and not vertical:
                if hz < 0.22 and int(wx // 5) % 2 == 0:
                    value = 240
                if 8.2 < hz < 8.45:
                    value = 241
            pixels.append(value)
    key = bytes(pixels)
    if key in cache:
        return cache[key]
    name = f"dm_road{len(cache)}.pcx"
    header = bytearray(128)
    header[:4] = bytes((10, 5, 1, 8))
    struct.pack_into("<4H", header, 4, 0, 0, 63, 63)
    header[65] = 1
    struct.pack_into("<HH", header, 66, 64, 1)
    palette = bytearray(c for i in range(256) for c in (i, i, i))
    palette[720:723] = bytes((224, 177, 66))
    palette[723:726] = bytes((165, 166, 155))
    encoded = bytearray()
    for value in pixels:
        if value >= 192:
            encoded.append(193)
        encoded.append(value)
    (folder / name).write_bytes(header + encoded + b"\x0c" + palette)
    cache[key] = name
    return name


def build(out, source):
    if out == source or out in source.parents:
        raise ValueError("The arena output must be separate from the game directory")
    for required in (
        "BHunter-source.exe",
        "BHunterGPU.dll",
        "SDL3.dll",
        "GAME_DATA/Bhunter.bin",
    ):
        if not (source / required).is_file():
            raise FileNotFoundError(
                f"Build the game first: missing {source / required}"
            )
    out.mkdir(parents=True, exist_ok=True)
    bsp = out / "bsp"
    bsp.mkdir(exist_ok=True)
    textures = out / "textures"
    textures.mkdir(exist_ok=True)
    road_tiles = {}
    placements, models, portals = [], [], []
    names = []

    def register(name, position, flags=0, script="grid 1\ncollision on\nrender on\n"):
        if name not in names:
            names.append(name)
            (bsp / name).with_suffix(".txt").write_text(script, encoding="ascii")
        placements.append((name, *position, flags))

    # Nine rooms share full-height portals. Buildings stay inside their owning room.
    for iz in range(3):
        for ix in range(3):
            x0, z0 = -120 + 80 * ix, -120 + 80 * iz
            x1, z1 = x0 + 80, z0 + 80
            name = f"dm_cell{iz * 3 + ix}.bsp"
            m = Model()
            for x in range(x0, x1, 10):
                for z in range(z0, z1, 10):
                    material = (
                        road_texture(textures, x, z, road_tiles)
                        if street(x + 5, z + 5)
                        else "P_FLOOR2.PCX"
                    )
                    m.rectangle(1, 0, x, x + 10, z, z + 10, -1, material, 10)
            m.rectangle(1, -100, x0, x1, z0, z1, 1, "ROOF.GIF", 1000)
            for axis, plane, a, b, c, d, sign, neighbor in [
                (0, x0, -100, 0, z0, z1, 1, ix > 0),
                (0, x1, -100, 0, z0, z1, -1, ix < 2),
                (2, z0, x0, x1, -100, 0, 1, iz > 0),
                (2, z1, x0, x1, -100, 0, -1, iz < 2),
            ]:
                m.rectangle(
                    axis,
                    plane,
                    a,
                    b,
                    c,
                    d,
                    sign,
                    "PORTAL.GIF" if neighbor else "DT_OLD01.PCX",
                    1000 if neighbor else 20,
                )
                if neighbor:
                    portals.append(
                        {
                            "cell": iz * 3 + ix,
                            "axis": axis,
                            "plane": plane,
                            "range": [a, b, c, d],
                            "normal": sign,
                        }
                    )
            models.append(m.write(bsp / name, inward=True))
            register(name, (0, 0, 0))

    buildings = []
    styles = ["DT_BIG01.PCX", "DT_OLD01.PCX", "F_GLAS01.PCX", "I_BWIN.PCX"]
    for i, (x, z, height) in enumerate(
        [
            (-80, -80, 50),
            (0, -80, 30),
            (80, -80, 62),
            (-80, 0, 36),
            (80, 0, 46),
            (-80, 80, 60),
            (0, 80, 34),
            (80, 80, 48),
        ]
    ):
        name = f"dm_house{i}.bsp"
        models.append(solid(bsp / name, 40, 40, height, styles[i % 4]))
        register(name, (x, 0, z))
        buildings.append(
            {"model": name, "center": [x, z], "size": [40, 40], "height": height}
        )
    for i, (x, z) in enumerate([(-20, -20), (20, -20), (-20, 20), (20, 20)]):
        name = "dm_cover.bsp"
        if i == 0:
            models.append(solid(bsp / name, 12, 12, 7, "I_BWALL1.PCX"))
        register(name, (x, 0, z))

    # Location scripts are executed by the stock loader, including the 8 multiplayer starts.
    spawns = [
        (-38, -12, -80, 0),
        (38, -12, -80, 0),
        (80, -12, -38, 3),
        (80, -12, 38, 3),
        (38, -12, 80, 2),
        (-38, -12, 80, 2),
        (-80, -12, 38, 1),
        (-80, -12, -38, 1),
    ]
    for i, (x, y, z, yaw) in enumerate(spawns):
        name = f"dm_start{i}.bsp"
        models.append(solid(bsp / name, 1, 1, 1, "I_BWALL2.PCX"))
        script = "grid 1\ncollision off\nrender off\n" + "".join(
            f"location {j} 20 0\n" for j in range(i, 32, 8)
        )
        register(name, (x, y, z), yaw, script)

    # The LEV ring assigns a prop to the preceding portal segment, not by bounds.
    rooms, props = placements[:9], placements[9:]
    placements = []
    for cell, room in enumerate(rooms):
        placements.append(room)
        for prop in props:
            ix = min(2, max(0, int((prop[1] + 120) // 80)))
            iz = min(2, max(0, int((prop[3] + 120) // 80)))
            if iz * 3 + ix == cell:
                placements.append(prop)
    (bsp / "blocks.txt").write_text("\n".join(names + ["end"]) + "\n", encoding="ascii")
    lev = pack("I", len(placements)) + b"".join(
        name.encode().ljust(16, b"\0") + pack("4i", *pose) for name, *pose in placements
    )
    (out / "test.lev").write_bytes(lev)
    (out / "crossroads.lev").write_bytes(lev)
    (out / "test.pth").write_bytes(pack("I", 0xFFFFFFFF))
    manifest = {
        "name": "Crossroads",
        "size": [240, 240],
        "flight_ceiling": 100,
        "buildings": buildings,
        "spawns": spawns,
        "models": models,
        "placements": placements,
        "portals": portals,
        "features": [
            "central court with four low cover blocks",
            "inner road loop and outer service road",
            "eight buildings with varied rooftop heights",
            "eight multiplayer spawn positions",
        ],
        "mode": "Deathmatch geometry and stock multiplayer spawns; no single-player mission or bots.",
    }
    (out / "arena.json").write_text(json.dumps(manifest, indent=2) + "\n")
    # Own runtime and settings: the normal game directory is not altered.
    for filename in ("BHunter-source.exe", "BHunterGPU.dll", "SDL3.dll"):
        shutil.copy2(source / filename, out / filename)
    if (source / "LICENCES").is_file():
        shutil.copy2(source / "LICENCES", out / "LICENCES")
    for folder in ("GAME_DATA", "MUSIC_DATA", "shaders"):
        if (source / folder).exists():
            shutil.copytree(source / folder, out / folder, dirs_exist_ok=True)
    for filename in ("source-preview.cfg", "game-config.ini"):
        if (source / filename).exists() and not (out / filename).exists():
            shutil.copy2(source / filename, out / filename)
    (out / "play.cmd").write_text(
        '@echo off\ncd /d "%~dp0"\n"BHunter-source.exe" --asset-dir "%~dp0." --settings-dir "%~dp0." --host\n',
        encoding="ascii",
    )
    (out / "join.cmd").write_text(
        '@echo off\ncd /d "%~dp0"\nset "arena_host=%~1"\nif not defined arena_host set /p "arena_host=Host IP: "\n"BHunter-source.exe" --asset-dir "%~dp0." --settings-dir "%~dp0." --join "%arena_host%"\n',
        encoding="ascii",
    )
    print(
        f"Built {out}: {len(placements)} placements, {len(portals) // 2} portal links, 8 starts"
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, default=ROOT / "build/test-arena")
    parser.add_argument("--game-dir", type=Path, default=ROOT / "build")
    args = parser.parse_args()
    build(args.output_dir.resolve(), args.game_dir.resolve())
