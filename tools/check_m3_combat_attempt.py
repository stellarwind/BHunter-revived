"""Audit original target motion separately from the unsuccessful combat policy."""
import json
import math
import struct
from check_m3_startup_evidence import ROOT, read, sha, events, event


def decode(words):
    return tuple(struct.unpack('<f', struct.pack('<I', w))[0] for w in words)


def main():
    path = 'META/m3/startup/454bf71975bd.json'
    r = read(path)
    assert not r['passed'] and not r['m3_complete'] and r['play_trace'] == 'mission-combat'
    assert r['requested_menu_path_observed'] and r['observe_callbacks'] == 1800
    assert [(c['label'], c['exit_code']) for c in r['commands']] == [('compile', 0), ('link', 0), ('startup', 1)]
    assert not r['original_file_patches'] and not r['visual_ui_tooling_used'] and not r['desktop_input_sent']
    assert r['registry_before'] == r['registry_after'] and r['private_key_absent_after']
    assert r['protected_windows_files_before'] == r['protected_windows_files_after']
    for p, h in r['source_hashes'].items():
        assert sha(r['source_snapshots'][p]) == h
    for p, h in r['original_hashes'].items():
        assert sha(p) == h
    contract = read('META/m3/combat-contract.json')
    for p, h in contract['evidence_sha256'].items():
        assert sha(p) == h
    assert not contract['source_game_admission'] and not contract['original_mapper_expanded']
    es = events(r)
    allocated = event(r, 'training-target-allocated')
    launched = event(r, 'training-launch-return')
    target = allocated['entity']
    for e in (allocated, launched):
        assert e['entity'] == target and e['owner'] == allocated['owner']
        assert (e['type'], e['bounty'], e['speed_bits'], e['death_event']) == (6, 5000, 0x41200000, 2013)
    states = [e for e in es if e['event'] == 'training-target-state']
    assert states[0]['phase'] == 'constructor-finished'
    assert states[0]['raw_fields'][:8] == [6, 5000, 2013, 0x41200000, 0x4398c0, 0x43a1d0, allocated['owner'], allocated['owner']]
    assert any(e['raw_fields'][7] != allocated['owner'] for e in states[1:])
    positions = [e for e in es if e['event'] == 'training-query-position']
    assert len(positions) == 32 and all(e['entity'] == target for e in positions)
    assert [e['completed_simulation_callbacks'] for e in positions] == list(range(655, 687))
    decoded = [decode(e['raw_position_words']) for e in positions]
    assert decoded[0] == (770.0, 20.0, -440.0) and len(set(decoded)) == 32
    assert all(math.isfinite(v) for p in decoded for v in p)
    # Printed coordinates are for readability; raw float words are authoritative.
    assert all(abs(v - printed) < 0.000001 for e, p in zip(positions, decoded) for v, printed in zip(p, e['position']))
    displacement = math.dist(decoded[0], decoded[-1])
    assert displacement > 6
    policies = [e for e in es if e['event'] == 'combat-policy']
    estimates = [e for e in es if e['event'] == 'combat-aim-estimate']
    assert len(policies) == len(estimates) == 191
    assert [e['completed_simulation_callbacks'] for e in policies] == list(range(656, 1797, 6))
    assert all(e['phase'] == 1 and not e['fire'] and e['ammo'] == 300 for e in policies)
    assert [e['completed_simulation_callbacks'] for e in policies if e['keys'][4]] == [848, 884]
    assert all(e['entity'] == target and not e['original_position_observation'] for e in estimates)
    assert min(e['player_position'][1] for e in policies) > -80
    keys = [e for e in es if e['event'] == 'child-key-message']
    assert all(e['posted'] and e['child_pid'] == e['verified_window_owner'] for e in keys)
    assert not any(e['down'] and e['virtual_key'] == 17 and e['after_simulation_callbacks'] > 655 for e in keys)
    forbidden = ('training-combat-boundary', 'training-damage-entry', 'player-ion-projectile-created',
                 'training-kill-broadcast-return', 'training-target-released', 'exception', 'timeout', 'probe-error')
    assert not any(e['event'] in forbidden for e in es)
    assert event(r, 'observation-boundary')['completed_simulation_callbacks'] == 1800
    assert event(r, 'breakpoints-restored')['verified'] and event(r, 'private-key-cleanup')['result'] == 0
    assert event(r, 'process-exit')['probe_termination'] and not event(r, 'probe-complete')['passed']
    assert all(e['x87_control'] == 0x27f for e in es if 'x87_control' in e)
    control = read('META/m3/combat-policy-control.json')
    assert control['passed'] and not control['original_code_executed'] and control['game_launches'] == 0
    assert len(control['commands']) == 3 and all(c['exit_code'] == 0 for c in control['commands'])
    for p, h in control['source_hashes'].items():
        assert sha(control['source_snapshots'][p]) == h
    assert all(control['source_hashes'][p] == h for p, h in r['source_hashes'].items())
    for f in r['profile_files']:
        assert sha(r['profile_path'] + '/' + f['name']) == f['sha256']
    assert sha(r['compatibility_log']['path']) == r['compatibility_log']['sha256']
    result = dict(passed=True, scope='evidence audit; combat scenario failed', run=r['run_id'],
                  evidence_sha256=sha(path), scenario_passed=False, target_spawn_verified=True,
                  original_query_positions=len(positions), position_callbacks=[655, 686],
                  observed_displacement=displacement, evaluated_target_motion_verified=True,
                  diagnostic_policy_phase=1, thrust_pulses=2, fire_input_sent=False, damage_validated=False,
                  kill_validated=False, source_game_admission=False, m3_complete=False)
    (ROOT / 'META/m3/combat-attempt-validation.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))


if __name__ == '__main__':
    main()
