"""Audit static evidence against immutable EXE bytes; never launches the game."""
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def main():
    c = json.loads((ROOT / 'META/m3/failure-contract.json').read_text())
    exe = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == c['original_exe_sha256']
    for p, h in c['evidence_sha256'].items():
        assert hashlib.sha256((ROOT / p).read_bytes()).hexdigest() == h, p
    pe = struct.unpack_from('<I', exe, 0x3c)[0]
    sections, optional_size = struct.unpack_from('<H', exe, pe + 6)[0], struct.unpack_from('<H', exe, pe + 20)[0]
    image_base = struct.unpack_from('<I', exe, pe + 24 + 28)[0]
    table = pe + 24 + optional_size
    def raw(va, size):
        rva = va - image_base
        for i in range(sections):
            vs, start, length, offset = struct.unpack_from('<IIII', exe, table + 40 * i + 8)
            if start <= rva and rva + size <= start + length:
                return exe[offset + rva - start:offset + rva - start + size]
        raise AssertionError(hex(va))
    count = 0
    for p in (ROOT / 'META/m3/failure-review').glob('*.instructions.json'):
        for ins in json.loads(p.read_text()):
            expected = bytes.fromhex(ins['bytes'])
            assert raw(int(ins['address'], 16), len(expected)) == expected, ins
            count += 1
    for p, number in [('00489220', 1.0), ('00489580', -0.2)]:
        data = json.loads((ROOT / ('META/m3/failure-review/' + p + '.memory.json')).read_text())[0]
        b = bytes.fromhex(data['hex'])
        assert raw(int(p, 16), 8) == b and struct.unpack('<d', b)[0] == number
    assert not c['runtime_failure_retry_captured'] and not c['game_source_admission']
    print(json.dumps(dict(passed=True, instruction_records_verified=count, game_launches=0, runtime_failure_retry_validated=False, m3_complete=False)))

if __name__ == '__main__':
    main()
