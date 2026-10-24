"""Audit keyboard-only flight into a real original mission sensor."""
import json
import math
from check_m3_startup_evidence import ROOT, read, sha, events, event

def main():
    r = read('META/m3/startup/14bdee1a5329.json')
    assert r['passed'] and r['play_trace'] == 'mission-approach' and r['requested_menu_path_observed']
    assert r['original_code_executes'] and r['original_file_patches'] == 0
    assert r['registry_before'] == r['registry_after'] and r['private_key_absent_after']
    assert r['protected_windows_files_before'] == r['protected_windows_files_after']
    for p, h in r['original_hashes'].items():
        assert sha(p) == h
    for p, h in r['source_hashes'].items():
        assert sha(r['source_snapshots'][p]) == h
    assert 'tools/m3_flight_probe.h' in r['source_hashes']
    policy = (ROOT / r['source_snapshots']['tools/m3_flight_probe.h']).read_text()
    assert not any(name in policy for name in ('write_bytes', 'WriteProcessMemory', 'VirtualAllocEx', 'SetThreadContext'))
    assert r['input_injection_used'] and not r['desktop_input_sent'] and not r['visual_ui_tooling_used']
    assert all(c['exit_code'] == 0 for c in r['commands'])
    assert event(r, 'episode-action-entry')['episode_index'] == 0 and event(r, 'player-reset-entry')['spawn_index'] == 21
    target = event(r, 'intro-flight-target')
    assert target['placement'] == 4939 and target['owner'] and target['bounds_owner']
    bounds = target['bounds']
    assert bounds == [-90, -25, 30, -30, -10, 90]
    centre = [(bounds[i] + bounds[i + 3]) / 2 for i in range(3)]
    calibration = [e for e in events(r) if e['event'] == 'flight-calibration']
    assert [e['axis'] for e in calibration] == ['yaw', 'pitch']
    assert all(abs(e['response']) > 0.00001 and e['sign'] == (1 if e['response'] > 0 else -1) for e in calibration)
    observations = [e for e in events(r) if e['event'] == 'flight-observation']
    assert [e['completed_simulation_callbacks'] for e in observations] == list(range(410, 543, 6))
    assert observations[0]['position'] == [0, -14, 120]
    assert all(abs(math.dist(e['position'], centre) - e['distance']) < 0.005 for e in observations)
    assert any(all(bounds[i] <= e['position'][i] <= bounds[i + 3] for i in range(3)) for e in observations)
    loaded = event(r, 'loaded-config-memory')
    profile = bytes.fromhex(loaded['profiles'][0]['record_hex'])
    keys = list(profile[0x1c:0x22])
    assert keys == [40, 38, 37, 39, 65, 90]
    injected = [e for e in events(r) if e['event'] == 'child-key-message']
    assert all(e['posted'] and e['child_pid'] == e['verified_window_owner'] for e in injected)
    held = [0] * 6
    for observation in observations:
        count = observation['completed_simulation_callbacks']
        for message in injected:
            if message['after_simulation_callbacks'] == count:
                assert message['virtual_key'] in keys
                held[keys.index(message['virtual_key'])] = int(message['down'])
        assert held == observation['keys']
    mission = [e for e in events(r) if e['event'] == 'mission-event']
    assert [e['id'] for e in mission] == [9991, 3332, 2010]
    assert mission[-1]['sender'] == target['owner'] and mission[-1]['completed_simulation_callbacks'] == 544
    stop = event(r, 'intro-sensor-boundary')
    assert stop['owner'] == target['owner'] and stop['handler_returned'] and stop['completed_simulation_callbacks'] == 544
    assert event(r, 'breakpoints-restored')['verified'] and event(r, 'private-key-cleanup')['result'] == 0
    assert event(r, 'process-exit')['probe_termination'] and event(r, 'process-exit')['code'] == 0x4d330101
    assert not any(e['event'] in ('timeout', 'exception', 'probe-error', 'original-winmain-exit-boundary') for e in events(r))
    assert all(e['x87_control'] == 0x27f for e in events(r) if 'x87_control' in e)
    for f in r['profile_files']:
        assert sha(r['profile_path'] + '/' + f['name']) == f['sha256']
    assert sha(r['compatibility_log']['path']) == r['compatibility_log']['sha256']
    print(json.dumps(dict(passed=True, run=r['run_id'], target_placement=4939, original_intro_event=2010, event_callback=544, original_handler_returned=True, flight_observations=len(observations), gameplay_state_patched=False, mission_completion_validated=False, destruction_retry_validated=False, m3_complete=False)))

if __name__ == '__main__':
    main()
