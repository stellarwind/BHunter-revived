"""Audit static tutorial-target evidence; does not launch or execute game code."""
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    contract = json.loads((ROOT / 'META/m3/target-contract.json').read_text())
    exe = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    manifest = json.loads((ROOT / 'META/reference-manifest.json').read_text())
    expected = next(f['sha256'] for f in manifest['files']
                    if f['path'] == 'GAME_DATA/BHUNTER.EXE')
    assert hashlib.sha256(exe).hexdigest() == expected == contract['exe_sha256']
    pe = struct.unpack_from('<I', exe, 0x3c)[0]
    base = struct.unpack_from('<I', exe, pe + 52)[0]
    table = pe + 24 + struct.unpack_from('<H', exe, pe + 20)[0]

    def raw(address, size):
        rva = address - base
        for i in range(struct.unpack_from('<H', exe, pe + 6)[0]):
            _, start, length, offset = struct.unpack_from('<IIII', exe, table + 40 * i + 8)
            if start <= rva and rva + size <= start + length:
                return exe[offset + rva - start:offset + rva - start + size]
        raise AssertionError(hex(address))

    for path, digest in contract['evidence_sha256'].items():
        assert hashlib.sha256((ROOT / path).read_bytes()).hexdigest() == digest, path
    assignment = json.loads((ROOT / 'META/m3/target-review/assignment.json').read_text())
    owners = {f['address']: f for f in assignment['functions']}
    for body in contract['bodies']:
        address = int(body['address'], 16)
        end = int(body['end'], 16)
        assert hashlib.sha256(raw(address, end - address + 1)).hexdigest() == body['sha256']
        assert owners[body['address']]['boundary'] == body['address'] + '-' + body['end']
        assert owners[body['address']]['implementation'] == 'not_started'
    for anchor in contract['instruction_anchors']:
        expected_bytes = bytes.fromhex(anchor['bytes'])
        assert raw(int(anchor['address'], 16), len(expected_bytes)) == expected_bytes
        asm = (ROOT / anchor['listing']).read_text()
        assert anchor['address'] + ' ' + anchor['instruction'] in asm
    for constant in contract['constants']:
        fmt = '<' + constant['format']
        assert struct.unpack(fmt, raw(int(constant['address'], 16), struct.calcsize(fmt)))[0] == constant['value']
    # Check dispatch data separately: these spans are data, not recovered functions.
    for dispatch in contract['dispatch_tables']:
        data = raw(int(dispatch['address'], 16), dispatch['size'])
        assert data.hex() == dispatch['bytes']
    case_map = raw(0x43a904, 21)
    event_targets = struct.unpack('<12I', raw(0x43a8d4, 48))
    assert event_targets[case_map[0]] == 0x43a6e9
    assert event_targets[case_map[3]] == 0x43a385
    # The constructor decrements the type before indexing the six-entry table.
    assert raw(0x43ae3e, 4) == bytes.fromhex('4f83ff05')
    assert struct.unpack('<I', raw(0x43b2ac + (6 - 1) * 4, 4))[0] == 0x43b14e
    route = json.loads((ROOT / 'META/m3/tutorial-route.json').read_text())
    slow = next(p for p in route['placements'] if p['placement'] == 4827)
    assert slow['slots'] == [2011, 1, 2013, 6, 5, 10, 0, 0]
    assert contract['expected_slow_target_arguments'] == {
        'placement': 4827, 'type': 6, 'bounty': 5000,
        'speed': 10.0, 'death_event': 2013,
    }
    assert not contract['runtime_spawn_validated']
    assert not contract['runtime_damage_validated']
    assert not contract['runtime_kill_validated']
    assert not contract['source_game_admission']
    report = dict(passed=True, scope='static tutorial target contract only',
                  bodies=len(contract['bodies']), instruction_anchors=len(contract['instruction_anchors']),
                  constants=len(contract['constants']), dispatch_tables=len(contract['dispatch_tables']),
                  game_launches=0, runtime_spawn_validated=False,
                  runtime_damage_validated=False, runtime_kill_validated=False, m3_complete=False)
    (ROOT / 'META/m3/target-contract-validation.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
