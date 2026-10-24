"""Independent VC5 rand/IMUL and PC53 timer storage controls."""
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def main():
    cases = [(1, 0, 0), (1, 1, 4), (23, -8, 3), (0xdeadbeef, 2, 19),
             (7919, 100000, -100000), (7, -2147483648, 2147483647)]
    rows = ['/* Independent VC5 random state, signed IMUL wrapping, PC53 stores. */']
    report = []
    for seed, low, high in cases:
        rng = seed
        values = []
        for i in range(2):
            rng = (rng * 214013 + 2531011) & 0xffffffff
            random = (rng >> 16) & 32767
            product = (random * ((high - low) & 0xffffffff)) & 0xffffffff
            signed = product if product < 0x80000000 else product - 0x100000000
            part = signed * (-1 / 32768)
            value = f32(low - part) if i == 0 else f32((f32(-.25 - 1 / 30) - part) + low)
            values.append(bits(value))
        rows.append('{%dUL,%s,%s,%dUL,%dUL},' %
                    (seed, '(-2147483647L-1)' if low == -2147483648 else str(low) + 'L',
                     str(high) + 'L', *values))
        report.append(dict(seed=seed, low=low, high=high, initial_bits=values[0], rearmed_bits=values[1]))
    header = ('\n'.join(rows) + '\n').encode()
    (ROOT / 'tests/script_timer_cases.h').write_bytes(header)
    evidence = dict(cases=report, header_sha256=hashlib.sha256(header).hexdigest(),
                    limit='Instruction-derived source controls, not original gameplay execution', game_launches=0)
    (ROOT / 'META/m4/script-timers/fixtures.json').write_text(json.dumps(evidence, indent=2) + '\n')
    print('Six independent timer random/storage controls generated.')


if __name__ == '__main__':
    main()
