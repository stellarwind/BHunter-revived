"""Independent portal/route query controls from existing startup observations."""
import hashlib
import json
import math
import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def f32(x):
    return struct.unpack('<f', struct.pack('<f', x))[0]


def center_normal(words):
    direction, plane = words[:2]
    if plane >= 0x80000000:
        plane -= 0x100000000
    extents = struct.unpack('<4f', struct.pack('<4I', *words[10:14]))
    u, v = f32((extents[0] + extents[1]) * .5), f32((extents[2] + extents[3]) * .5)
    axis = direction & 3
    point = [u, v]
    point.insert(axis, f32(plane))
    normal = [0., 0., 0.]
    normal[axis] = -1. if direction & 4 else 1.
    return point, normal


def plan(goal, center, normal):
    delta = [goal[i] - center[i] for i in range(3)]
    delta[1], delta[2] = f32(delta[1]), f32(delta[2])
    dot = (normal[0] * delta[0] + normal[1] * delta[1]) + normal[2] * delta[2]
    projected = [f32(dot * normal[0] + center[0]), f32(f32(dot) * normal[1] + center[1]), f32(f32(dot) * normal[2] + center[2])]
    def squared(d):
        return (d[0] * f32(d[0]) + d[1] * f32(d[1])) + d[2] * f32(d[2])
    a = math.sqrt(squared(delta))
    b = math.sqrt(squared([center[i] - projected[i] for i in range(3)]))
    rate = f32(4.666666666666667 / (a + b))
    return [*goal, *center, *projected, *center, rate]


def main():
    source = ROOT / 'META/m3/startup/2e901f78ca70.json'
    record = json.loads(source.read_text())
    events = record.get('events') or [json.loads(line) for c in record['commands'] if c['label'] == 'startup'
                                    for line in c['output'].splitlines() if line.startswith('{')]
    ports = sorted((p for p in events if p['event'] == 'navigation-port'), key=lambda p: (p['node'], p['slot']))
    payload = b''.join(struct.pack('<6f', *center_normal(p['raw_words'])[0], *center_normal(p['raw_words'])[1]) for p in ports)
    route = json.loads((ROOT / 'META/m3/navigation-route.json').read_text())
    goal = list(map(f32, route['goal_bounds_center']))
    by_port = {(p['node'], p['slot']): p for p in ports}
    rows = ['/* Independent 00433f70/00434040/00429480 instruction-derived output controls. */',
            '#define PORTAL_QUERY_CRC %dUL' % (zlib.crc32(payload) & 0xffffffff),
            '/* source node, goal node, exit slot, next node; then goal/center/projected/anchor/rate bits */',
            'static const unsigned long route_query_cases[4][17]={']
    for decision in route['original_matrix_decisions']:
        p = by_port[decision['node'], decision['exit_slot']]
        center, normal = center_normal(p['raw_words'])
        values = plan(goal, center, normal)
        bits = list(struct.unpack('<13I', struct.pack('<13f', *values)))
        words = [decision['node'], route['goal_node'], decision['exit_slot'], p['neighbor_index'], *bits]
        rows.append('{' + ','.join('0x%xUL' % v for v in words) + '},')
    rows.append('};')
    data = ('\n'.join(rows) + '\n').encode()
    (ROOT / 'tests/route_query_cases.h').write_bytes(data)
    report = dict(portals=len(ports), route_cases=4, portal_crc32=zlib.crc32(payload) & 0xffffffff,
                  source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(), header_sha256=hashlib.sha256(data).hexdigest(),
                  game_launches=0, scope='Static instruction/data-derived outputs, not gameplay or new original-code execution')
    (ROOT / 'META/m4/navigation-consumers/fixtures.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
