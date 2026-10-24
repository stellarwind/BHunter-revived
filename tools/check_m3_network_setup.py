"""Audit successful session-list trace independently of earlier stalled variant."""
import json
from check_m3_startup_evidence import ROOT, read, sha, events, event

def main():
    r = read('META/m3/startup/c41fc2a39889.json')
    assert r['passed'] and r['requested_menu_path_observed'] and r['child_window_input_trace'] == 'network-tcpip'
    assert r['original_file_patches'] == 0 and not r['visual_ui_tooling_used'] and not r['desktop_input_sent']
    for p, h in r['source_hashes'].items():
        assert sha(r['source_snapshots'][p]) == h
    for p, h in r['original_hashes'].items():
        assert sha(p) == h
    assert r['registry_before'] == r['registry_after'] and r['private_key_absent_after']
    assert r['protected_windows_files_before'] == r['protected_windows_files_after']
    assert all(c['exit_code'] == 0 for c in r['commands'])
    seed = read('META/m3/network-seed.json')
    assert r['renderer_preset']['seed_sha256'] == r['renderer_preset']['initial_cfg_sha256'] == seed['fixture_sha256']
    assert bytes.fromhex(event(r, 'loaded-config-memory')['settings_hex']) == (ROOT / seed['fixture']).read_bytes()[:416]
    assert event(r, 'network-setup-entry')['address'] == '127.0.0.1'
    assert event(r, 'network-setup-entry')['type'] == 1
    assert event(r, 'original-initialize-connection-return')['hresult'] == 0
    assert event(r, 'original-enum-sessions-return')['hresult'] == 0
    assert event(r, 'network-setup-return')['eax'] == 1
    assert event(r, 'network-setup-return')['sessions'] == 0
    ready = next(e for e in events(r) if e['event'] == 'menu-input-precondition' and e['after_simulation_callbacks'] == 500)
    assert ready['global_slot'] == 0x4d1488 and ready['selection'] == 1 and ready['closing'] == 0 and ready['step_callback'] == 0x42b2e0
    assert event(r, 'observation-boundary')['completed_simulation_callbacks'] == 600
    final = next(e for e in events(r) if e['event'] == 'menu-inventory' and e['phase'] == 'observation-stop' and e['global_slot'] == 0x4d1488)
    assert final['step_callback'] == 0x42b2e0 and final['selection_raw'] == 1
    assert event(r, 'breakpoints-restored')['verified'] and event(r, 'private-key-cleanup')['result'] == 0
    assert event(r, 'process-exit')['probe_termination']
    assert not any(e['event'] in ('timeout', 'exception', 'probe-error', 'unexpected-network-session-action') for e in events(r))
    names = {e['path'].lower().replace('\\', '/').split('/')[-1] for e in events(r) if e['event'] == 'module-loaded'}
    assert {'dplayx.dll', 'dpwsockx.dll', 'dsound.dll', 'ntdll.dll'} <= names
    for e in events(r):
        if e['event'] == 'child-key-message':
            assert e['posted'] and e['child_pid'] == e['verified_window_owner']
    control = read('META/m3/timeout-control.json')
    assert control['passed'] and all(c['exit_code'] == 0 for c in control['commands'])
    for p, h in control['sources_sha256'].items():
        assert sha(control['source_snapshots'][p]) == h
    assert sha(control['executable_path']) == control['executable_sha256']
    result = event(control, 'timeout-control-result')
    assert result['contexts_read'] and result['breakpoint_armed'] and result['restored_verified'] and not result['original_code_executes']
    contexts = [e for e in events(control) if e['event'] == 'stopped-thread-context']
    assert len(contexts) >= 3 and all(e['context_read'] and e['stack_read'] and e['eip'] for e in contexts)
    assert event(control, 'control-child-exit')['code'] == 0x4d33c001
    for f in r['profile_files']:
        assert sha(r['profile_path'] + '/' + f['name']) == f['sha256']
    assert sha(r['compatibility_log']['path']) == r['compatibility_log']['sha256']
    print(json.dumps(dict(passed=True, run=r['run_id'], original_tcpip_setup=True, session_list_active=True, sessions_found=0, callbacks=600, earlier_stall_explained=False, timeout_capture_control_passed=True, host_join_peer_validated=False, m3_complete=False)))

if __name__ == '__main__':
    main()
