"""Audit the corrected forward convention without accepting failed combat."""
import json
import math
from check_m3_startup_evidence import ROOT, read, sha, event, events


def main():
    path = 'META/m3/startup/469b5ddce4f6.json'
    r = read(path)
    assert not r['passed'] and not r['m3_complete'] and r['play_trace'] == 'mission-combat'
    assert [(c['label'], c['exit_code']) for c in r['commands']] == [('compile', 0), ('link', 0), ('startup', 1)]
    assert r['requested_menu_path_observed'] and r['observe_callbacks'] == 1800
    assert not r['original_file_patches'] and not r['visual_ui_tooling_used'] and not r['desktop_input_sent']
    assert r['registry_before'] == r['registry_after'] and r['private_key_absent_after']
    assert r['protected_windows_files_before'] == r['protected_windows_files_after']
    for p, h in r['source_hashes'].items():
        assert sha(r['source_snapshots'][p]) == h
    for p, h in r['original_hashes'].items():
        assert sha(p) == h
    control = read('META/m3/combat-direction-control.json')
    assert control['passed'] and not control['native_probe_executed']
    assert all(control['source_hashes'][p] == h for p, h in r['source_hashes'].items())
    es = events(r)
    allocation = event(r, 'training-target-allocated')
    launch = event(r, 'training-launch-return')
    assert allocation['entity'] == launch['entity'] and allocation['owner'] == launch['owner']
    assert (launch['type'], launch['bounty'], launch['speed_bits'], launch['death_event']) == (6, 5000, 0x41200000, 2013)
    policies = [e for e in es if e['event'] == 'combat-policy']
    assert len(policies) == 191
    assert [e['completed_simulation_callbacks'] for e in policies] == list(range(656, 1797, 6))
    assert all(e['phase'] == 2 and e['policy_forward_convention'] == 'positive' and not e['fire'] and e['ammo'] == 300 for e in policies)
    assert sum(e['keys'][4] for e in policies) == 181
    dot = lambda a, b: sum(x * y for x, y in zip(a, b))
    for e in policies:
        for field in ('basis_forward', 'basis_up', 'basis_right', 'gun_direction', 'velocity', 'player_position'):
            assert len(e[field]) == 3 and all(math.isfinite(x) for x in e[field])
        assert e['gun_direction'] == e['basis_forward']
        assert abs(dot(e['velocity'], e['basis_forward']) - e['forward_velocity']) < 0.0001
        for field in ('basis_forward', 'basis_up', 'basis_right'):
            assert abs(dot(e[field], e[field]) - 1) < 0.0001
    minimum = min(e['distance_estimate'] for e in policies)
    assert minimum > 870 and policies[-1]['gun_alignment'] > 0.97
    # Actual command storage and verified child messages support the sign review.
    keys = [e for e in es if e['event'] == 'child-key-message']
    assert all(e['posted'] and e['child_pid'] == e['verified_window_owner'] for e in keys)
    assert any(e['virtual_key'] == 65 and e['down'] and e['after_simulation_callbacks'] > 655 for e in keys)
    assert not any(e['virtual_key'] == 17 and e['down'] for e in keys)
    last = [e for e in es if e['event'] == 'player-state'][-1]
    assert last['completed_simulation_callbacks'] == 1800 and last['control_words'][2] == 0x3f800000
    assert (last['mode_raw'], last['pause_raw'], last['failure_raw']) == (1, 0, 0)
    forbidden = ('player-ion-projectile-created', 'training-damage-entry', 'training-kill-broadcast-return',
                 'training-target-released', 'training-combat-boundary', 'probe-error', 'timeout', 'exception')
    assert not any(e['event'] in forbidden for e in es)
    assert event(r, 'observation-boundary')['completed_simulation_callbacks'] == 1800
    assert event(r, 'breakpoints-restored')['verified'] and event(r, 'private-key-cleanup')['result'] == 0
    assert event(r, 'process-exit')['probe_termination'] and not event(r, 'probe-complete')['passed']
    for f in r['profile_files']:
        assert sha(r['profile_path'] + '/' + f['name']) == f['sha256']
    assert sha(r['compatibility_log']['path']) == r['compatibility_log']['sha256']
    result = dict(passed=True, scope='evidence audit; corrected combat approach still failed', run=r['run_id'],
                  evidence_sha256=sha(path), scenario_passed=False, positive_forward_projection_verified=True,
                  actual_gun_matches_positive_basis=True, thrust_policy_samples=181,
                  minimum_target_distance_estimate=minimum, fire_input_sent=False,
                  obstacle_or_contact_cause_verified=False, damage_validated=False, kill_validated=False,
                  source_game_admission=False, m3_complete=False)
    (ROOT / 'META/m3/forward-attempt-validation.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))


if __name__ == '__main__':
    main()
