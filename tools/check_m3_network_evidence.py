"""Audit partial original TCP/IP success and retained post-setup timeout."""
import json
from check_m3_startup_evidence import ROOT, read, sha, events, event

def main():
    r = read('META/m3/startup/a6528d0cdfed.json')
    assert not r['passed'] and r['child_window_input_trace'] == 'network-tcpip'
    assert r['original_code_executes'] and r['original_file_patches'] == 0
    assert not r['desktop_input_sent'] and not r['visual_ui_tooling_used']
    assert r['registry_before'] == r['registry_after'] and r['private_key_absent_after']
    assert r['protected_windows_files_before'] == r['protected_windows_files_after']
    for p, h in r['original_hashes'].items():
        assert sha(p) == h
    for p, h in r['source_hashes'].items():
        assert sha(r['source_snapshots'][p]) == h
    assert [(c['label'], c['exit_code']) for c in r['commands']] == [('compile', 0), ('link', 0), ('startup', 1)]
    seed = read('META/m3/network-seed.json')
    assert sha(seed['source']) == seed['source_sha256']
    assert sha(seed['fixture']) == seed['fixture_sha256']
    old, fixture = (ROOT / seed['source']).read_bytes(), (ROOT / seed['fixture']).read_bytes()
    assert len(old) == len(fixture) == 644
    assert [i for i, (a, b) in enumerate(zip(old, fixture)) if a != b] == list(range(140, 149))
    assert fixture[140:268] == b'127.0.0.1\0' + bytes(118)
    assert r['renderer_preset']['seed_sha256'] == r['renderer_preset']['initial_cfg_sha256'] == seed['fixture_sha256']
    loaded = event(r, 'loaded-config-memory')
    assert bytes.fromhex(loaded['settings_hex']) == fixture[:416]
    provider = event(r, 'original-tcpip-provider')
    assert provider['type'] == 1 and provider['menu_index'] == 3 and provider['name'] == 'TCP/IP Internet'
    entry = event(r, 'network-setup-entry')
    assert entry['provider_record'] == provider['record'] and entry['type'] == 1 and entry['address'] == '127.0.0.1'
    assert entry['completed_simulation_callbacks'] == 440
    assert event(r, 'original-initialize-connection-return')['hresult'] == 0
    assert event(r, 'original-enum-sessions-return')['hresult'] == 0
    assert event(r, 'original-enum-sessions-return')['sessions'] == 0
    result = event(r, 'network-setup-return')
    assert result['eax'] == 1 and result['directplay'] and result['lobby'] and result['sessions'] == 0
    event(r, 'timeout-context')
    event(r, 'timeout')
    assert event(r, 'process-exit')['probe_termination'] and event(r, 'process-exit')['code'] == 0x4d330102
    assert event(r, 'private-key-cleanup')['result'] == 0
    assert not any(e['event'] in ('unexpected-network-session-action', 'observation-boundary', 'breakpoints-restored', 'exception', 'probe-error') for e in events(r))
    keys = [e for e in events(r) if e['event'] == 'child-key-message']
    assert all(e['child_pid'] == e['verified_window_owner'] and e['posted'] for e in keys)
    assert keys[-1]['after_simulation_callbacks'] == 440 and keys[-1]['virtual_key'] == 13
    assert all(e['x87_control'] == 0x27f for e in events(r) if 'x87_control' in e)
    for f in r['profile_files']:
        assert sha(r['profile_path'] + '/' + f['name']) == f['sha256']
    assert sha(r['compatibility_log']['path']) == r['compatibility_log']['sha256']
    print(json.dumps(dict(evidence_audit_passed=True, scenario_passed=False, run=r['run_id'], original_tcpip_initialize_hresult=0, original_session_enum_hresult=0, sessions_found=0, post_setup_timeout=True, restored_breakpoints_verified=False, host_join_executed=False, m3_complete=False)))

if __name__ == '__main__':
    main()
