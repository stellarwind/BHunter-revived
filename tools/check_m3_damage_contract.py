"""Verify archived static damage evidence against pinned original bytes."""
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def main():
    manifest = json.loads((ROOT / 'META/reference-manifest.json').read_text())
    exe = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == next(v['sha256'] for v in manifest['files'] if v['path'] == 'GAME_DATA/BHUNTER.EXE')
    pe = struct.unpack_from('<I', exe, 0x3c)[0]
    table = pe + 24 + struct.unpack_from('<H', exe, pe + 20)[0]
    count = struct.unpack_from('<H', exe, pe + 6)[0]
    base = struct.unpack_from('<I', exe, pe + 52)[0]
    def raw(va, size):
        for i in range(count):
            _, start, length, offset = struct.unpack_from('<IIII', exe, table + i * 40 + 8)
            if start <= va - base and va - base + size <= start + length:
                return exe[offset + va - base - start:offset + va - base - start + size]
        raise AssertionError(hex(va))
    c = json.loads((ROOT / 'META/m3/damage-contract.json').read_text())
    for p, h in c['evidence_sha256'].items():
        assert hashlib.sha256((ROOT / p).read_bytes()).hexdigest() == h
    total = 0
    for p in (ROOT / 'META/m3/damage-review').glob('*.instructions.json'):
        for ins in json.loads(p.read_text()):
            b = bytes.fromhex(ins['bytes'])
            assert raw(int(ins['address'], 16), len(b)) == b
            total += 1
    for name, value in c['constants'].items():
        actual = struct.unpack('<f' if name == '0048930c' else '<d', raw(int(name, 16), 4 if name == '0048930c' else 8))[0]
        assert actual == value, (name, actual)
    assert not c['runtime_destruction_validated'] and not c['source_game_admission']
    print(json.dumps(dict(passed=True, instruction_records=total, constants=len(c['constants']), game_launches=0, runtime_destruction_validated=False, m3_complete=False)))

if __name__ == '__main__':
    main()
