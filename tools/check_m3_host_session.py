"""Audit actual original DirectPlay host creation; never claims peers/gameplay."""
import json
from check_m3_startup_evidence import ROOT, read, sha, events, event

def main():
    r = read('META/m3/startup/04a72c7a5e54.json')
    assert r['passed'] and r['requested_menu_path_observed'] and r['child_window_input_trace'] == 'network-host'
    assert r['original_code_executes'] and r['original_file_patches'] == 0
    assert not r['visual_ui_tooling_used'] and not r['desktop_input_sent']
    assert r['registry_before'] == r['registry_after'] and r['private_key_absent_after']
    assert r['protected_windows_files_before'] == r['protected_windows_files_after']
    for p, h in r['source_hashes'].items():
        assert sha(r['source_snapshots'][p]) == h
    for p, h in r['original_hashes'].items():
        assert sha(p) == h
    assert all(c['exit_code'] == 0 for c in r['commands'])
    seed = read('META/m3/network-seed.json')
    assert r['renderer_preset']['seed_sha256'] == r['renderer_preset']['initial_cfg_sha256'] == seed['fixture_sha256']
    assert bytes.fromhex(event(r, 'loaded-config-memory')['settings_hex']) == (ROOT / seed['fixture']).read_bytes()[:416]
    assert event(r, 'network-setup-entry')['type'] == 1 and event(r, 'network-setup-entry')['address'] == '127.0.0.1'
    assert event(r, 'original-initialize-connection-return')['hresult'] == 0
    assert event(r, 'original-enum-sessions-return')['hresult'] == 0
    assert event(r, 'original-new-game-entry')['completed_simulation_callbacks'] == 520
    opened = event(r, 'original-host-open-call')
    assert opened['flags'] == 0x82 and opened['session_flags'] == 0xc4 and opened['max_players'] == 8 and opened['current_players'] == 0
    assert event(r, 'original-host-open-return')['hresult'] == 0
    assert event(r, 'original-host-create-player-call')['flags'] == 0x100
    player = event(r, 'original-host-create-player-return')
    assert player['hresult'] == 0 and player['player_id'] == 1
    helper = event(r, 'original-host-helper-return')
    assert helper['eax'] == 1 and helper['host_ready'] == 1 and helper['local_player_id'] == 1
    boundary = event(r, 'network-host-boundary')
    assert boundary['mode_raw'] == 2 and boundary['host_ready'] == 1 and boundary['local_player_id'] == 1 and boundary['completed_simulation_callbacks'] == 520
    assert event(r, 'breakpoints-restored')['verified'] and event(r, 'private-key-cleanup')['result'] == 0
    assert event(r, 'process-exit')['probe_termination'] and event(r, 'process-exit')['code'] == 0x4d330101
    assert not any(e['event'] in ('unexpected-network-session-action', 'timeout', 'exception', 'probe-error', 'observation-boundary') for e in events(r))
    keys = [e for e in events(r) if e['event'] == 'child-key-message']
    assert keys[-1]['after_simulation_callbacks'] == 520 and keys[-1]['virtual_key'] == 13
    assert all(e['posted'] and e['child_pid'] == e['verified_window_owner'] for e in keys)
    assert all(e['x87_control'] == 0x27f for e in events(r) if 'x87_control' in e)
    for f in r['profile_files']:
        assert sha(r['profile_path'] + '/' + f['name']) == f['sha256']
    assert sha(r['compatibility_log']['path']) == r['compatibility_log']['sha256']
    print(json.dumps(dict(passed=True, run=r['run_id'], original_host_open_hresult=0, original_server_player_hresult=0, local_player_id=1, requested_capacity=8, host_mode=2, join_peer_exchange_validated=False, original_session_shutdown_validated=False, m3_complete=False)))

if __name__ == '__main__':
    main()
