"""Generate independent BSP face/vertex controls from immutable asset data."""
import argparse
import hashlib
import json
import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def sections(data):
    at = 0
    result = []
    for stride in (32, 60, 16, 12, 56, 8):
        count, = struct.unpack_from('<I', data, at)
        at += 4
        result.append([data[i:i + stride] for i in range(at, at + count * stride, stride)])
        at += count * stride
    assert at == len(data)
    return result


def controls(data):
    materials, vertices, edges, faces, nodes, leaves = sections(data)
    edges = [struct.unpack('<iiff', e) for e in edges]
    faces = [struct.unpack('<iii', f) for f in faces]
    nodes = [struct.unpack('<10f4i', n) for n in nodes]
    leaves = [struct.unpack('<ii', l) for l in leaves]
    # First predecessor is significant for shared tails.
    predecessor = {}
    for i, (_, nxt) in enumerate(leaves):
        predecessor.setdefault(nxt, i)
    face_bytes, vertex_bytes = bytearray(), bytearray()
    normal = (0., 0., 0.)
    missing = 0
    for face_id, (material, first, _) in enumerate(faces):
        chain, edge = [], first
        while edge >= 0:
            chain.append(edges[edge][0])
            edge = edges[edge][1]
            assert len(chain) <= len(edges)
        assigned = False
        for i, (candidate, _) in enumerate(leaves):
            if candidate != face_id:
                continue
            root = i
            while root in predecessor:
                root = predecessor[root]
            for n in nodes:
                if n[10] == root:
                    normal, assigned = n[6:9], True
                elif n[11] == root:
                    normal, assigned = tuple(-x for x in n[6:9]), True
        missing += not assigned
        face_bytes.extend(struct.pack('<III', face_id, len(chain), 0))
        face_bytes.extend(materials[material][9:])
        face_bytes.extend(vertices[chain[0]][:12])
        face_bytes.extend(struct.pack('<3f', *normal))
        vertex_bytes.extend(b''.join(vertices[v][:12] for v in chain))
    return zlib.crc32(face_bytes) & 0xffffffff, zlib.crc32(vertex_bytes) & 0xffffffff, missing


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write', action='store_true')
    args = parser.parse_args()
    archive = (ROOT / 'GAME_DATA/Bhunter.bin').read_bytes()
    pinned = json.loads((ROOT / 'META/reference-manifest.json').read_text())
    assert hashlib.sha256(archive).hexdigest() == next(f['sha256'] for f in pinned['files'] if f['path'] == 'GAME_DATA/Bhunter.bin')
    index = json.loads((ROOT / 'analysis/assets-index.json').read_text())
    rows, missing = [], 0
    for item in index:
        if not item['name'].upper().endswith('.BSP'):
            continue
        face_crc, vertex_crc, absent = controls(archive[item['offset']:item['end']])
        missing += absent
        rows.append('{' + json.dumps(item['name']) + ',%dUL,%dUL,%dUL},' % (face_crc, vertex_crc, absent))
    content = '/* Independent raw-BSP face/edge/normal controls. */\n' + '\n'.join(rows) + '\n'
    header = ROOT / 'tests/geometry_cases.h'
    if args.write:
        header.write_text(content, newline='\n')
    else:
        assert header.read_text() == content
    report = dict(bsp_files=len(rows), faces_without_assigned_normal=missing,
                  fixture_sha256=hashlib.sha256(content.encode()).hexdigest(),
                  archive_sha256=hashlib.sha256(archive).hexdigest(), game_launches=0,
                  scope='Independent topology interpreter; normal carry starts at zero for each source geometry owner; not original code execution')
    folder = ROOT / 'META/m4/construction'
    if args.write:
        (folder / 'geometry-fixtures.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
