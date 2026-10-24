"""Audit isolated host availability results; capability is not game validation."""
import json
import struct
from check_m3_startup_evidence import ROOT, read, sha, events, event
from m3_startup_probe import imports

def main():
    r = read('META/m3/host/662c8a7b39f4.json')
    assert r['passed'] and not r['original_code_executes']
    assert r['registry_before'] == r['registry_after']
    for p, h in r['original_hashes'].items():
        assert sha(p) == h
    for p, h in r['source_hashes'].items():
        assert sha(p) == sha(r['source_snapshots'][p]) == h
    assert sha(r['sdk_dplay_header']['path']) == r['sdk_dplay_header']['sha256']
    assert sha(r['executable_path']) == r['executable_sha256']
    assert [c['label'] for c in r['commands']] == ['compile', 'link', 'host-query']
    assert all(c['exit_code'] == 0 for c in r['commands'])
    assert event(r, 'host-api-probe')['pointer_bits'] == 32
    assert not any(e['event'] == 'cdrom-drive' for e in events(r))
    assert event(r, 'mci-cdaudio-open')['error'] == 266  # MCIERR_CANNOT_LOAD_DRIVER
    assert event(r, 'multimedia-device-counts')['aux'] == 0
    assert event(r, 'directplay3a-create')['hresult'] == 0
    assert event(r, 'directplay3a-create')['interface_created']
    assert event(r, 'directplay-enumeration')['hresult'] == 0
    providers = [e for e in events(r) if e['event'] == 'directplay-provider']
    assert len(providers) == event(r, 'directplay-enumeration')['providers'] == 3
    assert {e['guid'] for e in providers} == {'0f1d6860-88d9-11cf-9c4e-00a0c905425e', '44eaa760-cb68-11cf-9c4e-00a0c905425e', '36e95ee0-8577-11cf-960c-0080c7534e82'}
    done = event(r, 'host-probe-complete')
    assert not any(done[k] for k in ('playback_requested', 'volume_write_requested', 'network_session_requested'))
    imported = imports((ROOT / r['executable_path']).read_bytes())
    assert not {'auxSetVolume', 'waveOutSetVolume', 'mciSendStringA'} & imported.keys()
    assert {'mciSendCommandA', 'CoCreateInstance', 'CoInitialize', 'CoUninitialize'} <= imported.keys()
    # Confirm the original uses the same COM class/interface as the SDK probe.
    b = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    pe = struct.unpack_from('<I', b, 0x3c)[0]
    table = pe + 24 + struct.unpack_from('<H', b, pe + 20)[0]
    count = struct.unpack_from('<H', b, pe + 6)[0]
    base = struct.unpack_from('<I', b, pe + 52)[0]
    def raw(va):
        for i in range(count):
            _, start, size, offset = struct.unpack_from('<IIII', b, table + i * 40 + 8)
            if start <= va - base and va - base + 16 <= start + size:
                return b[offset + va - base - start:offset + va - base - start + 16]
        raise AssertionError(hex(va))
    assert raw(0x48a088).hex() == '41fe3e13dc32d0119cfb00a0c90a43cb'
    assert raw(0x48a098).hex() == '206debd12389d0119d9700a0c90a43cb'
    print(json.dumps(dict(passed=True, cdaudio_open_error=266, cdrom_drives=0, directplay3a_available=True, providers=[e['name'] for e in providers], playback_validated=False, network_sessions_validated=False, m3_complete=False)))

if __name__ == '__main__':
    main()
