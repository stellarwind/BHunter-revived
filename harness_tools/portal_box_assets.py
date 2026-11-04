"""Generate standalone BSP/LEV box assets with finite doorway apertures."""

import json
import struct
from pathlib import Path


def pack(fmt, *values):
    return struct.pack("<" + fmt, *values)


def write_box(path, low, high, doors):
    materials = [
        bytes(9) + name.ljust(23, b"\0") for name in (b"solid.pcx", b"PORTAL.GIF")
    ]
    vertices, edges, faces, normals, leaves = [], [], [], [], []
    for axis in range(3):
        u, v = [index for index in range(3) if index != axis]
        for side in range(2):
            plane = (low, high)[side][axis]
            rectangles = [(low[u], high[u], low[v], high[v], 0)]
            if (axis, side) in doors:
                a, b, c, d = doors[axis, side]
                rectangles = [
                    (low[u], a, low[v], high[v], 0),
                    (b, high[u], low[v], high[v], 0),
                    (a, b, low[v], c, 0),
                    (a, b, d, high[v], 0),
                    (a, b, c, d, 1),
                ]
            for a, b, c, d, material in rectangles:
                if a == b or c == d:
                    continue
                normal = [0.0] * 3
                normal[axis] = 1.0 if side == 0 else -1.0
                points = []
                for first, second in ((a, c), (b, c), (b, d), (a, d)):
                    point = [0.0] * 3
                    point[axis], point[u], point[v] = plane, first, second
                    points.append(point)
                first = [points[1][i] - points[0][i] for i in range(3)]
                second = [points[2][i] - points[1][i] for i in range(3)]
                cross = [
                    first[1] * second[2] - first[2] * second[1],
                    first[2] * second[0] - first[0] * second[2],
                    first[0] * second[1] - first[1] * second[0],
                ]
                if sum(cross[i] * normal[i] for i in range(3)) < 0:
                    points.reverse()
                base = len(vertices)
                for index, point in enumerate(points):
                    vertices.append(pack("3f", *point) + bytes(48))
                    edges.append(
                        pack(
                            "iiff",
                            base + index,
                            base + index + 1 if index < 3 else -1,
                            float(index in (1, 2)),
                            float(index >= 2),
                        )
                    )
                faces.append(pack("iii", material, base, 0))
                normals.append((normal, -normal[axis] * plane))
                leaves.append(pack("ii", len(faces) - 1, -1))
    nodes = [
        pack(
            "10f4i",
            *low,
            *high,
            *normal,
            plane,
            index,
            index,
            index + 1 if index + 1 < len(faces) else -1,
            -1,
        )
        for index, (normal, plane) in enumerate(normals)
    ]
    path.write_bytes(
        b"".join(
            pack("I", len(rows)) + b"".join(rows)
            for rows in (materials, vertices, edges, faces, nodes, leaves)
        )
    )


def generate(out):
    bsp = out / "bsp"
    bsp.mkdir(exist_ok=True)
    aperture = (-4, 4, -3, 3)
    rooms = [
        ("box_court.bsp", (-8, -8, -20), (8, 8, 0), {(2, 1): aperture}),
        (
            "box_station.bsp",
            (-8, -8, 0),
            (8, 8, 20),
            {(2, 0): aperture, (2, 1): aperture},
        ),
        ("box_airlock.bsp", (-8, -8, 20), (8, 8, 40), {(2, 0): aperture}),
    ]
    for name, low, high, doors in rooms:
        write_box(bsp / name, low, high, doors)
        (bsp / Path(name).with_suffix(".txt")).write_text(
            "grid 1\ncollision on\nrender on\n"
        )
    (bsp / "blocks.txt").write_text("\n".join(name for name, *_ in rooms) + "\nend\n")
    (out / "test.lev").write_bytes(
        pack("i", len(rooms))
        + b"".join(
            name.encode().ljust(16, b"\0") + pack("4i", 0, 0, 0, 0)
            for name, *_ in rooms
        )
    )
    manifest = {
        "kind": "isolated police-entrance box visibility diagnostic",
        "assets": [
            {
                "model": name,
                "minimum": low,
                "maximum": high,
                "doorways": [
                    {"axis": axis, "side": side, "aperture": rectangle}
                    for (axis, side), rectangle in doors.items()
                ],
            }
            for name, low, high, doors in rooms
        ],
        "coverage": [
            "all six entrance directions",
            "both crossing directions",
            "exact portal equality",
            "near plane",
            "aperture edges and solid wall controls",
            "nested airlock",
            "four-room cyclic corner",
            "interpolation samples",
            "4:3, 16:9 and ultrawide bounds",
        ],
        "limits": "Targeted finite scenarios, not every possible level. BSP/LEV assets plus standalone visibility runner; not a mission replacement or full GPU/model-renderer playthrough.",
        "retail_assets_modified": False,
    }
    (out / "level.json").write_text(json.dumps(manifest, indent=2) + "\n")
