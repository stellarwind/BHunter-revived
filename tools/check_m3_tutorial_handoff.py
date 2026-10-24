"""Audit uninterrupted original sensor route through training handoff."""
import json
import hashlib
import struct
from check_m3_startup_evidence import ROOT, read, sha, events, event

def main():
    r = read('META/m3/startup/80df3c4686e1.json')
    assert r['passed'] and r['play_trace'] == 'mission-tour' and r['requested_menu_path_observed']
    assert r['original_file_patches'] == 0 and not r['visual_ui_tooling_used'] and not r['desktop_input_sent']
    assert r['registry_before'] == r['registry_after'] and r['private_key_absent_after']
    assert r['protected_windows_files_before'] == r['protected_windows_files_after']
    for p, h in r['source_hashes'].items():
        assert sha(r['source_snapshots'][p]) == h
    for p, h in r['original_hashes'].items():
        assert sha(p) == h
    assert all(c['exit_code'] == 0 for c in r['commands'])
    targets = [e for e in events(r) if e['event'] == 'intro-flight-target']
    assert [e['placement'] for e in targets] == [4939, 5055, 5064]
    waypoints = [e for e in events(r) if e['event'] == 'intro-waypoint-return']
    assert [(e['stage'], e['placement'], e['id'], e['completed_simulation_callbacks']) for e in waypoints] == [(0,4939,2010,544), (1,5055,1100,601), (2,5064,2011,655)]
    mission = [e for e in events(r) if e['event'] == 'mission-event']
    assert [e['id'] for e in mission] == [9991,3332,2010,1100,2011]
    for i, target in enumerate(targets):
        assert target['owner'] == waypoints[i]['owner'] == mission[i + 2]['sender']
        start = 410 if i == 0 else waypoints[i - 1]['completed_simulation_callbacks']
        end = waypoints[i]['completed_simulation_callbacks']
        positions = [e['position'] for e in events(r) if e['event'] == 'flight-observation' and start <= e['completed_simulation_callbacks'] <= end]
        for e in events(r):
            if e['event'] == 'player-state' and start <= e['completed_simulation_callbacks'] <= end:
                positions.append([struct.unpack('<f',struct.pack('<I',w))[0] for w in e['position_words']])
        bounds = target['bounds']
        assert any(all(bounds[j] <= p[j] <= bounds[j+3] for j in range(3)) for p in positions)
    observations = [e for e in events(r) if e['event'] == 'flight-observation']
    assert len(observations) == 41 and observations[0]['position'] == [0,-14,120]
    calibration = [e for e in events(r) if e['event'] == 'flight-calibration']
    assert [e['axis'] for e in calibration] == ['yaw','pitch']
    assert all(abs(e['response']) > 0.00001 and e['sign'] == (1 if e['response'] > 0 else -1) for e in calibration)
    keys = [40,38,37,39,65,90]
    injected = [e for e in events(r) if e['event'] == 'child-key-message']
    assert all(e['posted'] and e['child_pid'] == e['verified_window_owner'] for e in injected)
    for observation in observations:
        held = [0] * 6
        for message in injected:
            if 410 <= message['after_simulation_callbacks'] <= observation['completed_simulation_callbacks']:
                assert message['virtual_key'] in keys
                held[keys.index(message['virtual_key'])] = int(message['down'])
        assert held == observation['keys']
    stop = event(r, 'intro-sensor-boundary')
    assert stop['route_stages'] == 3 and stop['handler_returned'] and stop['completed_simulation_callbacks'] == 655
    assert event(r, 'breakpoints-restored')['verified'] and event(r, 'private-key-cleanup')['result'] == 0
    assert event(r, 'process-exit')['probe_termination']
    assert not any(e['event'] in ('timeout','exception','probe-error','original-winmain-exit-boundary') for e in events(r))
    assert all(e['x87_control'] == 0x27f for e in events(r) if 'x87_control' in e)
    for f in r['profile_files']:
        assert sha(r['profile_path']+'/'+f['name']) == f['sha256']
    assert sha(r['compatibility_log']['path']) == r['compatibility_log']['sha256']
    route = read('META/m3/tutorial-route.json')
    archive = (ROOT/'GAME_DATA/Bhunter.bin').read_bytes()
    index = read('analysis/assets-index.json')
    level = next(e for e in index if e['name'].upper() == 'TEST.LEV')
    data = archive[level['offset']:level['end']]
    assert hashlib.sha256(data).hexdigest() == route['level_sha256'] == '1da23c4537d874b15a40093038d2fec9bd8da53b83260efed5b4469c0bcf7df7'
    count = struct.unpack_from('<I',data)[0]
    placements = list(struct.iter_unpack('<16siiiI',data[4:4+count*32]))
    links = list(struct.iter_unpack('<I8h',data[4+count*32:]))
    for candidate in route['placements']:
        ordinal = candidate['placement']
        name,x,y,z,flags = placements[ordinal]
        assert candidate['name'] == name.rstrip(b'\0').decode() and candidate['position'] == [x,y,z] and candidate['flags'] == flags
        assert [list(link[1:]) for link in links if link[0] == ordinal] == [candidate['slots']]
        script = next(e for e in index if e['name'] == candidate['script_name'])
        text = archive[script['offset']:script['end']]
        assert hashlib.sha256(text).hexdigest() == candidate['script_sha256'] and text.decode('latin1') == candidate['script_text']
    briefing = route['training_briefing']
    text = briefing['raw_text'].encode('latin1')
    assert briefing['event_id'] == 2011 and hashlib.sha256(text).hexdigest() == briefing['sha256']
    exe = (ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert exe.count(text+b'\0') == 1
    position = exe.index(text+b'\0')
    pe = struct.unpack_from('<I',exe,0x3c)[0]
    table = pe+24+struct.unpack_from('<H',exe,pe+20)[0]
    sections = struct.unpack_from('<H',exe,pe+6)[0]
    base = struct.unpack_from('<I',exe,pe+52)[0]
    addresses = []
    for i in range(sections):
        _,va,size,offset = struct.unpack_from('<IIII',exe,table+i*40+8)
        if offset <= position < offset+size:
            addresses.append(base+va+position-offset)
    assert addresses == [int(briefing['original_address'],16)]
    print(json.dumps(dict(passed=True, run=r['run_id'], original_events=[2010,1100,2011], original_handlers_returned=3, flight_observations=41, target_spawn_or_kill_validated=False, episode_completion_validated=False, m3_complete=False)))

if __name__ == '__main__':
    main()
