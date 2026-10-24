"""Independent instruction-derived navigation controls from saved startup data."""
import hashlib
import json
import math
import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def f32(x):
    return struct.unpack('<f', struct.pack('<f', x))[0]


def main():
    source = ROOT / 'META/m3/startup/2e901f78ca70.json'
    record = json.loads(source.read_text())
    events = record.get('events') or [json.loads(line) for c in record['commands'] if c['label'] == 'startup'
                                    for line in c['output'].splitlines() if line.startswith('{')]
    nodes = {n['index']: n for n in events if n['event'] == 'navigation-node'}
    bounds = {i: [f32(x) for x in n['bounds']] for i, n in nodes.items()}
    ports = [[None] * 6 for _ in range(1454)]
    for p in events:
        if p['event'] == 'navigation-port' and p['neighbor']:
            ports[p['node']][p['slot']] = p['neighbor_index']
    endpoints = [[0, 0, 0] for _ in range(1454)]
    def distance(previous, current):
        a, b = bounds[previous], bounds[current]
        d = [(b[i] + b[i + 3]) * .5 - f32((a[i] + a[i + 3]) * .5) for i in range(3)]
        return f32(math.sqrt((d[0] * f32(d[0]) + d[1] * f32(d[1])) + d[2] * f32(d[2])))
    def walk(origin, previous, current):
        stack = []
        while True:
            edge = distance(previous, current)
            if current < 283:
                length, target = edge, current
                break
            seen, nxt = 0, None
            for candidate in ports[current]:
                if candidate is None:
                    continue
                if candidate != previous:
                    slot = 0 if seen else 1
                    endpoints[current][slot] = origin
                    endpoints[current][2] |= 1 << slot
                    nxt = candidate
                    break
                seen += 1
            if nxt is None:
                length, target = 0., origin
                break
            stack.append(edge)
            assert len(stack) < 1454
            previous, current = current, nxt
        for edge in reversed(stack):
            length = f32(length + edge)
        return target, length
    count = 283
    sentinel = struct.unpack('<f', struct.pack('<I', 0x7e876abd))[0]
    lengths, exits = [sentinel] * (count * count), [-1] * (count * count)
    for i in range(count):
        lengths[i * count + i] = 0.
        for slot, neighbor in enumerate(ports[i]):
            if neighbor is not None:
                target, length = walk(i, i, neighbor)
                lengths[target * count + i], exits[target * count + i] = length, slot
    # Keep the original k/j/i traversal and symmetric paired stores. Compare the
    # double sum before binary32 rounding; rounded-equal improvements still write.
    for k in range(count):
        for j in range(count):
            for i in range(count):
                candidate = lengths[i * count + k] + lengths[k * count + j]
                if candidate < lengths[i * count + j]:
                    lengths[i * count + j] = lengths[j * count + i] = f32(candidate)
                    exits[i * count + j] = exits[k * count + j]
                    exits[j * count + i] = exits[k * count + i]
    payload = b''.join(struct.pack('<if', e, d) for e, d in zip(exits, lengths))
    rows = ['/* Independent instruction-derived no-cache matrix and corridor endpoints. */',
            '#define NAVIGATION_MATRIX_CRC %dUL' % (zlib.crc32(payload) & 0xffffffff),
            'static const unsigned long navigation_endpoints[1454][3]={']
    rows.extend('{%dUL,%dUL,%dUL},' % tuple(e) for e in endpoints)
    rows.append('};')
    header = ('\n'.join(rows) + '\n').encode()
    (ROOT / 'tests/navigation_cases.h').write_bytes(header)
    index = json.loads((ROOT / 'analysis/assets-index.json').read_text())
    item = next(x for x in index if x['name'].upper() == 'TEST.PTH')
    with (ROOT / 'GAME_DATA/Bhunter.bin').open('rb') as f:
        f.seek(item['offset'] + 4)
        cache = f.read(item['size'] - 4)
    differences = [i for i in range(count * count) if payload[i * 8:i * 8 + 8] != cache[i * 8:i * 8 + 8]]
    cache_exits, cache_lengths = zip(*struct.iter_unpack('<if', cache))
    report = dict(nodes=1454, junctions=count, cells=count * count,
                  matrix_crc32=zlib.crc32(payload) & 0xffffffff,
                  recomputed_vs_legacy_different_cells=len(differences),
                  recomputed_vs_legacy_different_exits=sum(a != b for a, b in zip(exits, cache_exits)),
                  recomputed_vs_legacy_max_distance_difference=max(abs(a - b) for a, b in zip(lengths, cache_lengths)),
                  source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                  header_sha256=hashlib.sha256(header).hexdigest(), game_launches=0,
                  scope='Instruction-derived Python binary32/PC53 model, not original-code execution; legacy PTH is retained separately')
    (ROOT / 'META/m4/navigation/fixtures.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
