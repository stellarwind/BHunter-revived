"""Independent PC53 arithmetic controls for the packed world-position decoder."""
import hashlib
import json
import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def append(crc, bounds, words):
    for axis, word in enumerate(words):
        t = (word - 16384.0) / 32768.0
        value = bounds[axis + 3] * t + bounds[axis] * (1.0 - t)
        crc = zlib.crc32(struct.pack('<f', value), crc)
    return crc


def main():
    synthetic = 0
    bounds = [-123.125, 0.0009765625, -10000000000.0,
              456.75, 98765.5, 10000001024.0]
    for word in range(65536):
        synthetic = append(synthetic, bounds, [word, word ^ 0x5555, 65535 - word])
    path = ROOT / 'META/m3/startup/2e901f78ca70.json'
    saved = json.loads(path.read_text())
    events = saved.get('events') or [json.loads(line) for c in saved['commands']
                                   if c['label'] == 'startup'
                                   for line in c['output'].splitlines() if line.startswith('{')]
    nodes = sorted((n for n in events if n['event'] == 'navigation-node'),
                   key=lambda n: n['index'])
    real = 0
    for i, node in enumerate(nodes):
        assert node['index'] == i
        bounds = [struct.unpack('<f', struct.pack('<f', x))[0] for x in node['bounds']]
        real = append(real, bounds, [(i * 997 + axis * 16384) & 65535 for axis in range(3)])
    header = ('/* Independent instruction-derived output CRCs; no original execution. */\n'
              '#define POSITION_SYNTHETIC_CRC %dUL\n#define POSITION_REAL_CRC %dUL\n'
              % (synthetic, real)).encode()
    (ROOT / 'tests/position_cases.h').write_bytes(header)
    report = dict(synthetic_records=65536, real_records=len(nodes),
                  synthetic_crc32=synthetic, real_crc32=real,
                  header_sha256=hashlib.sha256(header).hexdigest(),
                  reference_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                  reference=str(path.relative_to(ROOT)), game_launches=0,
                  limit='Instruction-derived arithmetic controls, not runtime decoder observations')
    (ROOT / 'META/m4/waypoint/fixtures.json').write_text(json.dumps(report, indent=2) + '\n')
    print('Position controls: 65536 synthetic records and %d saved world bounds.' % len(nodes))


if __name__ == '__main__':
    main()
