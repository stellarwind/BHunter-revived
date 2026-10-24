"""Audit real original quit/save -> original settings/profile reload evidence."""
import json
import struct
from check_m3_startup_evidence import ROOT, read, sha, events, event

def main():
    old = read('META/m3/startup/dfa3b4981226.json')
    r = read('META/m3/startup/c42a7051b694.json')
    assert old['passed'] and event(old, 'process-exit')['code'] == 0
    assert not event(old, 'process-exit')['probe_termination']
    seed = ROOT / r['renderer_preset']['seed_path']
    data = seed.read_bytes()
    assert len(data) == 644
    assert sha(seed) == old['profile_files'][0]['sha256'] == r['renderer_preset']['seed_sha256']
    assert r['renderer_preset']['initial_cfg_sha256'] == sha(seed)  # preset changed no input bytes
    assert r['passed'] and not r['through_bootstrap'] and not r['input_injection_used']
    assert not r['visual_ui_tooling_used'] and not r['desktop_input_sent']
    assert r['original_file_patches'] == 0 and r['original_hashes'] == old['original_hashes']
    for p, h in r['original_hashes'].items():
        assert sha(p) == h
    for p, h in r['source_hashes'].items():
        assert sha(r['source_snapshots'][p]) == h  # archived reload variant
    assert r['registry_before'] == r['registry_after'] and r['private_key_absent_after']
    assert r['protected_windows_files_before'] == r['protected_windows_files_after']
    assert event(r, 'source-check-return')['eax'] == 1
    assert event(r, 'render-mode-return')['eax'] == 1
    assert not event(r, 'bootstrap-entry')['game_bootstrap_executed']
    assert event(r, 'breakpoints-restored')['verified']
    assert event(r, 'private-key-cleanup')['result'] == 0
    assert event(r, 'process-exit')['probe_termination']  # reload stop is diagnostic, unlike preceding quit
    assert not any(e['event'] in ('timeout', 'exception', 'message-box-boundary', 'child-key-message') for e in events(r))
    loaded = event(r, 'loaded-config-memory')
    assert bytes.fromhex(loaded['settings_hex']) == data[:416]
    profiles = loaded['profiles']
    assert len(profiles) == 3 and loaded['head'] == profiles[0]['address']
    pointer_offsets = set(range(4, 12)) | set(range(72, 76))
    disk = (ROOT / r['profile_path'] / 'BHUNTER.CFG').read_bytes()
    assert sha(ROOT / r['profile_path'] / 'BHUNTER.CFG') == r['profile_files'][0]['sha256']
    # Reviewed 004022d0 temporarily zeros settings word 00496a38 for its save,
    # then restores the in-memory word. Audit that actual difference explicitly.
    assert disk[:4] == b'\0' * 4 and disk[4:416] == data[4:416] and len(disk) == len(data)
    names = []
    for i, profile in enumerate(profiles):
        b = bytes.fromhex(profile['record_hex'])
        original = data[416 + i * 76:416 + (i + 1) * 76]
        saved = disk[416 + i * 76:416 + (i + 1) * 76]
        flag, following, previous = struct.unpack_from('<3I', b)
        assert flag == 0 and following == profiles[(i + 1) % 3]['address'] and previous == profiles[(i - 1) % 3]['address']
        assert all(b[j] == original[j] == saved[j] for j in range(76) if j not in pointer_offsets)
        assert b[4:12] != original[4:12] and b[72:76] != original[72:76]
        assert struct.unpack_from('<I', b, 72)[0] != 0
        names.append(b[12:28].split(b'\0', 1)[0].decode('ascii'))
    assert names == ['<Standard>', '<MouseView>', '<Joy 1 View>']
    assert all(e['x87_control'] == 0x27f for e in events(r) if 'x87_control' in e)
    print(json.dumps(dict(passed=True, save_run=old['run_id'], reload_run=r['run_id'], settings_bytes_exact=416, profiles=names, pointer_ring_rebuilt=True, game_bootstrap_executed=False, m3_complete=False)))

if __name__ == '__main__':
    main()
