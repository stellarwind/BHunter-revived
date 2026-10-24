"""Audit actual spawn evidence retained from a failed longer target scenario."""
import json
import struct
from check_m3_startup_evidence import ROOT, read, sha, events, event


def floats(words):
    return [struct.unpack('<f', struct.pack('<I', w))[0] for w in words]


def main():
    path = 'META/m3/startup/df50ceaa4826.json'
    r = read(path)
    assert not r['passed'] and not r['m3_complete'] and r['play_trace'] == 'mission-target'
    assert r['requested_menu_path_observed']
    assert [(c['label'], c['exit_code']) for c in r['commands']] == [('compile', 0), ('link', 0), ('startup', 1)]
    assert not r['original_file_patches'] and not r['visual_ui_tooling_used'] and not r['desktop_input_sent']
    assert r['registry_before'] == r['registry_after'] and r['private_key_absent_after']
    assert r['protected_windows_files_before'] == r['protected_windows_files_after']
    for p, h in r['source_hashes'].items():
        assert sha(r['source_snapshots'][p]) == h
    for p, h in r['original_hashes'].items():
        assert sha(p) == h
    es = events(r)
    allocation = event(r, 'training-target-allocated')
    launch = event(r, 'training-launch-return')
    states = [e for e in es if e['event'] == 'training-target-state']
    assert len(states) == 2
    initial, advanced = states
    entity = allocation['entity']
    owner = allocation['owner']
    assert entity and owner and entity != owner
    for e in (allocation, launch):
        assert e['entity'] == entity and e['owner'] == owner
        assert (e['type'], e['bounty'], e['speed_bits'], e['death_event']) == (6, 5000, 0x41200000, 2013)
        assert e['completed_simulation_callbacks'] == 655
    assert initial['phase'] == 'constructor-finished' and initial['completed_simulation_callbacks'] == 655
    assert advanced['phase'] == 'simulation-return' and advanced['completed_simulation_callbacks'] == 660
    for e in states:
        fields = e['raw_fields']
        assert e['entity'] == entity
        assert fields[:7] == [6, 5000, 2013, 0x41200000, 0x4398c0, 0x43a1d0, owner]
        assert fields[11:] == [0, 0x3f800000, 0x3f800000, 0]
        assert e['raw_displacement_words'] == [0, 0, 0]
    assert initial['raw_fields'][7] == owner and advanced['raw_fields'][7] != owner
    assert initial['raw_fields'][9:11] == [0x3f800000, 0x3f800000]
    assert initial['raw_curve_words'] != advanced['raw_curve_words']
    assert floats(initial['raw_curve_words']) == [770.0, 20.0, -440.0] * 5
    assert initial['raw_fields'][8] != advanced['raw_fields'][8]
    mission = [e for e in es if e['event'] == 'mission-event']
    assert [e['id'] for e in mission] == [9991, 3332, 2010, 1100, 2011]
    handoff = event(r, 'training-handoff-continued')
    assert handoff['completed_simulation_callbacks'] == 655
    assert es.index(allocation) < es.index(initial) < es.index(launch) < es.index(mission[-1]) < es.index(handoff) < es.index(advanced)
    # The failure is the probe's overstrict pointer predicate, not an original fault.
    error = event(r, 'probe-error')
    assert error['kind'] == 17 and es.index(advanced) + 1 == es.index(error)
    assert not event(r, 'probe-complete')['passed']
    assert event(r, 'breakpoints-restored')['verified']
    assert event(r, 'private-key-cleanup')['result'] == 0
    assert event(r, 'process-exit')['probe_termination'] and event(r, 'process-exit')['code'] == 0x4d330107
    absent = ('training-render-position', 'training-target-boundary', 'training-damage-entry',
              'training-kill-broadcast-return', 'training-target-released', 'timeout', 'exception')
    assert not any(e['event'] in absent for e in es)
    held = {k: 0 for k in (40, 38, 37, 39, 65, 90)}
    for e in es:
        if e['event'] == 'child-key-message':
            assert e['posted'] and e['child_pid'] == e['verified_window_owner']
            if e['after_simulation_callbacks'] >= 410:
                assert e['virtual_key'] in held
                held[e['virtual_key']] = int(e['down'])
    assert not any(held.values())
    assert all(e['x87_control'] == 0x27f for e in es if 'x87_control' in e)
    for f in r['profile_files']:
        assert sha(r['profile_path'] + '/' + f['name']) == f['sha256']
    assert sha(r['compatibility_log']['path']) == r['compatibility_log']['sha256']
    build = read('META/m3/target-probe-build.json')
    assert build['passed'] and not build['original_code_executed'] and build['game_launches'] == 0
    assert len(build['commands']) == 2 and all(c['exit_code'] == 0 for c in build['commands'])
    for p, h in build['source_hashes'].items():
        assert sha(build['source_snapshots'][p]) == h
    assert build['source_hashes']['tools/m3_flight_probe.h'] != r['source_hashes']['tools/m3_flight_probe.h']
    result = dict(passed=True, scope='evidence audit, not full scenario acceptance', run=r['run_id'],
                  evidence_sha256=sha(path), scenario_passed=False, spawn_contract_verified=True,
                  constructor_callback=655, later_state_callback=660, navigation_pointer_changed=True,
                  curve_state_changed=True, evaluated_position_trace_validated=False,
                  damage_validated=False, kill_validated=False, game_launches=1, m3_complete=False)
    (ROOT / 'META/m3/target-spawn-validation.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))


if __name__ == '__main__':
    main()
