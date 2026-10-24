"""Check original forward-sign evidence and the corrected diagnostic controls."""
import json
import struct
from check_m3_startup_evidence import ROOT, read, sha, event, events


def main():
    contract = read('META/m3/forward-contract.json')
    exe = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert sha('GAME_DATA/BHUNTER.EXE') == contract['exe_sha256']
    manifest = read('META/reference-manifest.json')
    assert contract['exe_sha256'] == next(e['sha256'] for e in manifest['files'] if e['path'] == 'GAME_DATA/BHUNTER.EXE')
    pe = struct.unpack_from('<I', exe, 0x3c)[0]
    base = struct.unpack_from('<I', exe, pe + 52)[0]
    table = pe + 24 + struct.unpack_from('<H', exe, pe + 20)[0]

    def raw(address, size):
        for i in range(struct.unpack_from('<H', exe, pe + 6)[0]):
            _, start, length, offset = struct.unpack_from('<IIII', exe, table + 40 * i + 8)
            if start <= address - base and address - base + size <= start + length:
                return exe[offset + address - base - start:offset + address - base - start + size]
        raise AssertionError(hex(address))

    for p, h in contract['evidence_sha256'].items():
        assert sha(p) == h
    for a in contract['instruction_anchors']:
        data = bytes.fromhex(a['bytes'])
        assert raw(int(a['address'], 16), len(data)) == data
        assert a['address'] + ' ' + a['instruction'] in (ROOT / a['listing']).read_text()
    constants = {}
    for c in contract['constants']:
        fmt = '<' + c['format']
        value = struct.unpack(fmt, raw(int(c['address'], 16), struct.calcsize(fmt)))[0]
        assert value == c['value']
        constants[c['address']] = value
    assert constants == {'004896a0': -30.0, '004894f0': -10.0, '00489554': -150.0, '00489208': -1 / 30}
    # Derived local command examples, before the rest of the actuator solver.
    def commanded_forward(old, input_value):
        coefficient = constants['004894f0'] if old * input_value > 0 else constants['004896a0']
        return old - input_value * coefficient
    assert [commanded_forward(v, u) for v, u in [(0, 1), (1, 1), (0, -1), (-1, -1)]] == [30, 11, -30, -11]
    # Zero player speed / unit positive-forward gun: positive displacement.
    projectile_velocity = 0 - 1 * constants['00489554']
    assert projectile_velocity == 150 and 0 - projectile_velocity * constants['00489208'] == 5
    # Existing executed A-key evidence establishes the positive command input.
    old = read('META/m3/startup/dfa3b4981226.json')
    assert old['passed']
    assert any(e['event'] == 'player-state' and e['accelerate_key_state'] and e['control_words'][2] == 0x3f800000 for e in events(old))
    build = read('META/m3/combat-direction-control.json')
    assert build['passed'] and build['source_only_control_executed']
    assert not build['original_code_executed'] and not build['native_probe_executed'] and build['game_launches'] == 0
    for p, h in build['source_hashes'].items():
        assert sha(build['source_snapshots'][p]) == h
    labels = [c['label'] for c in build['commands']]
    assert labels == ['control-compile', 'control-link', 'control-execute', 'native-compile', 'native-link']
    assert all(c['exit_code'] == 0 for c in build['commands'])
    control = json.loads(build['commands'][2]['output'])
    assert control['passed'] and control['curve_coordinate_checks'] == 18
    assert control['forward_projection_checks'] == 4 and control['turn_direction_checks'] == 4
    assert not contract['corrected_policy_original_execution_validated'] and not contract['source_game_admission']
    result = dict(passed=True, game_launches=0, instruction_anchors=len(contract['instruction_anchors']),
                  constants=len(constants), original_forward_contract='positive basis.forward',
                  source_control_executed=True, corrected_native_probe_compiled_and_linked=True,
                  corrected_policy_original_execution_validated=False, source_game_admission=False, m3_complete=False)
    (ROOT / 'META/m3/forward-contract-validation.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))


if __name__ == '__main__':
    main()
