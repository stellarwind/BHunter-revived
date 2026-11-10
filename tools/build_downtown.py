"""Extract the retail Downtown district as a standalone BHunter arena."""

import argparse
import hashlib
import struct
from functools import lru_cache
from pathlib import Path
from tempfile import TemporaryDirectory

from pack_level import pack_level

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "build/GAME_DATA"
LEVEL_SHA256 = "1da23c4537d874b15a40093038d2fec9bd8da53b83260efed5b4469c0bcf7df7"
PLACEMENT = struct.Struct("<16s4i")
OVERRIDE = struct.Struct("<I8h")

# Retail TEST.LEV placement IDs, traced from Downtown's two CITYPART markers.
# Keep the street component, the connected Downtown metro loop, and the
# entrance rooms on the Downtown side of the cut connections.
STREET = {
    1105, 1117, 1124, 1133, 1144, 1150, 1162, 1173, 1184, 1210, 1221, 1230,
    1236, 1249, 1275, 1293, 1296, 1297, 1298, 1299, 1300, 1301, 1302, 1316,
    1317, 1318, 1326, 1333, 1374, 1376, 1380, 1381, 1382, 1383, 1384, 1385,
    1386, 1387, 1388, 1390, 1517, 3424, 3429, 3444, 3461, 3472, 3495, 3516,
    3523, 3534, 3545, 3562, 3578, 3593, 3597, 3601, 3614, 3617, 4176, 4183,
    4196, 4200, 4207, 4212, 4217, 4258, 4279, 4289, 4297, 4305, 4314, 4319,
    4326, 4343, 4350, 4355, 4361, 4369, 4792, 4823, 4829, 4836, 4871, 4877,
    4896, 4897, 4898, 4920,
}
METRO = {
    1349, 1350, 1352, 1354, 1371, 1406, 1408, 1409, 1410, 1411, 1412, 1413,
    1415, 1417, 1419, 1420, 1422, 1424, 1426, 1428, 1430, 1434, 1435, 1436,
    1437, 1438, 1439, 1440, 1441, 1442, 1443, 1444, 1445, 1446, 1447, 1448,
    1449, 1450, 1451, 1452, 1453, 1454, 1455, 1456, 1457, 1458, 1459, 1460,
    1461, 1462, 1463, 1464, 1465, 1466, 1467, 1516,
}
# The far portal on each entrance is converted to a collidable wall. The
# street-facing portal stays active so the entrance is still visible and usable.
# original BSP, new BSP, portal slot, existing wall material
SEWER = ("T_GUL_UD.BSP", "dd_sewer.bsp", 0, b"TUNGUL20.TGA")
RING = ("T_MOT_EX.BSP", "dd_ring.bsp", 0, b"TUNMOT10.TGA")
CAPS = {
    1101: SEWER,
    1302: ("TGRE_YEL.BSP", "dd_ramp.bsp", 0, b"TGRE_W2Y.TGA"),
    1366: ("T_MET_U1.BSP", "dd_metro.bsp", 1, b"TUNMET21.TGA"),
    1730: RING,
    1733: RING,
    4257: SEWER,
}
RING_DOORS = {1731, 1732, 1734, 1735}
METRO_DOORS = {1367, 1370}
# Open street positions near the middle of their rooms, facing a connected road.
STARTS = {
    1105: (125, 20, -215, 1),
    1124: (545, 20, -5, 3),
    1184: (965, 20, -215, 0),
    1236: (515, 20, -635, 1),
    1333: (620, 20, 460, 2),
    3424: (455, 20, 205, 0),
    4258: (965, 20, -5, 2),
    4792: (650, 20, -380, 0),
}


def archive_entries(path):
    data = path.read_bytes()
    count, = struct.unpack_from("<I", data)
    entries = {}
    for i in range(count):
        name, start, end = struct.unpack_from("<248sII", data, 4 + 256 * i)
        entries[name.split(b"\0", 1)[0].decode("ascii").upper()] = data[start:end]
    return entries


def bsp_tables(data):
    offset = 0
    for stride in (32, 60, 16, 12, 56, 8):
        count, = struct.unpack_from("<I", data, offset)
        offset += 4
        yield range(offset, offset + count * stride, stride)
        offset += count * stride
    if offset != len(data):
        raise ValueError("Invalid BSP table sizes")


def materials_and_portals(data):
    materials, _, _, faces, _, _ = bsp_tables(data)
    names = [data[i + 9:i + 32].split(b"\0", 1)[0].upper() for i in materials]
    portals = [i for i in faces if names[struct.unpack_from("<I", data, i)[0]] == b"PORTAL.GIF"]
    return names, portals


def close_portal(data, slot, texture):
    materials, portals = materials_and_portals(data)
    patched = bytearray(data)
    struct.pack_into("<I", patched, portals[slot], materials.index(texture))
    return patched


def ring_gate(data):
    # The retail panel is 15 units high. Fit it to the 20-unit ring opening
    # while retaining its texture coordinates and one-unit thickness.
    gate = bytearray(data)
    _, vertices, _, _, nodes, _ = bsp_tables(data)
    for i in vertices:
        y, = struct.unpack_from("<f", data, i + 4)
        struct.pack_into("<f", gate, i + 4, y * 4 / 3)
    for i in nodes:
        for offset in (4, 16):
            y, = struct.unpack_from("<f", data, i + offset)
            struct.pack_into("<f", gate, i + offset, y * 4 / 3)
        nx, ny, nz, distance = struct.unpack_from("<4f", data, i + 24)
        if ny:
            if nx or nz or abs(ny) != 1:
                raise ValueError("Unexpected retail gate plane")
            struct.pack_into("<f", gate, i + 36, distance * 4 / 3)
    return gate


def extract(archive, target):
    entries = archive_entries(archive)
    level = entries["TEST.LEV"]
    if hashlib.sha256(level).hexdigest() != LEVEL_SHA256:
        raise ValueError("This extraction requires the retail TEST.LEV layout")

    @lru_cache(maxsize=None)
    def is_room(name):
        return bool(materials_and_portals(entries["BSP\\" + name])[1])

    bsp = target / "bsp"
    bsp.mkdir()
    for original, name, slot, material in set(CAPS.values()):
        (bsp / name).write_bytes(close_portal(entries["BSP\\" + original], slot, material))
        script = entries.get("BSP\\" + original.replace(".BSP", ".TXT"))
        if script:
            (bsp / Path(name).with_suffix(".txt")).write_bytes(script)
    gate = entries["BSP\\GATE.BSP"]
    for name, model in (("dd_ringgate", ring_gate(gate)), ("dd_metgate", gate)):
        (bsp / (name + ".bsp")).write_bytes(model)
        (bsp / (name + ".txt")).write_bytes(b"grid 1\ncollision on\nrender on\n")

    (bsp / "location.txt").write_bytes(
        b"grid 1\ncollision off\nrender off\n"
        + b"".join(f"define slot{i} 0\n".encode("ascii") for i in range(4))
        + b"".join(f"location slot{i} 20 0\n".encode("ascii") for i in range(4))
    )

    rooms = STREET | METRO | CAPS.keys()
    placements, overrides, remap, models = [], [], {}, {}

    def place(name, pose):
        models.setdefault(name.upper(), name)
        placements.append(PLACEMENT.pack(name.encode("ascii"), *pose))

    count, = struct.unpack_from("<I", level)
    end = 4 + count * PLACEMENT.size
    owner = None
    kept_rooms = set()
    start_index = {room: i for i, room in enumerate(STARTS)}
    for i, (raw_name, x, y, z, flags) in enumerate(PLACEMENT.iter_unpack(level[4:end])):
        name = raw_name.split(b"\0", 1)[0].decode("ascii").upper()
        if is_room(name):
            owner = i
        if owner not in rooms or name == "LOCATION.BSP":
            continue
        if owner == i:
            kept_rooms.add(i)
        if i in CAPS:
            original, replacement, _, _ = CAPS[i]
            if name != original:
                raise ValueError(f"Unexpected room model at placement {i}")
            name = replacement
        if i in RING_DOORS:
            name, y = "dd_ringgate.bsp", 40
        elif i in METRO_DOORS:
            name = "dd_metgate.bsp"
        remap[i] = len(placements)
        place(name, (x, y, z, flags))
        if i in STARTS:
            slot = start_index[i]
            overrides.append(OVERRIDE.pack(len(placements), slot, slot + 8, slot + 16, slot + 24, 0, 0, 0, 0))
            place("LOCATION.BSP", STARTS[i])

    if kept_rooms != rooms:
        raise ValueError("A Downtown room is missing from the extraction")
    for i, *variables in OVERRIDE.iter_unpack(level[end:]):
        if i in remap and i not in RING_DOORS and i not in METRO_DOORS:
            overrides.append(OVERRIDE.pack(remap[i], *variables))
    (bsp / "blocks.txt").write_bytes(("\n".join(models.values()) + "\nend\n").encode("ascii"))
    (target / "level.lev").write_bytes(
        struct.pack("<I", len(placements)) + b"".join(placements) + b"".join(overrides)
    )
    return len(rooms), len(placements)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archive", type=Path, default=DATA / "Bhunter.bin")
    parser.add_argument("--output", type=Path, default=DATA / "downtown.bhlevel")
    args = parser.parse_args()
    with TemporaryDirectory() as workspace:
        source = Path(workspace)
        rooms, placements = extract(args.archive, source)
        pack_level(source, args.output)
    print(f"Built {args.output} with {rooms} rooms and {placements} placements")
