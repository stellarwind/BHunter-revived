"""Audit one batched original weapon/camera/event run; no replay."""
import json
from check_m3_startup_evidence import ROOT, read, sha, events, event

def main():
    r = read('META/m3/startup/5fc01fdfc4f6.json')
    assert r['passed'] and r['play_trace'] == 'features-quit' and not r['m3_complete']
    assert r['requested_menu_path_observed'] and r['original_code_executes'] and r['original_file_patches'] == 0
    assert r['registry_before'] == r['registry_after'] and r['private_key_absent_after']
    assert r['protected_windows_files_before'] == r['protected_windows_files_after']
    for p, h in r['original_hashes'].items():
        assert sha(p) == h
    for p, h in r['source_hashes'].items():
        assert sha(r['source_snapshots'][p]) == h  # archived executed feature variant
    assert r['relative_cursor_adapter'] and r['crt_initial_heap_mib_adapter'] == 128
    assert not r['desktop_input_sent'] and not r['visual_ui_tooling_used']
    assert all(c['exit_code'] == 0 for c in r['commands'])
    assert event(r, 'bootstrap-return')['eax'] == 1 and event(r, 'first-surface-flip-result')['hresult'] == 0
    assert event(r, 'player-reset-entry')['spawn_index'] == 21
    states = {e['completed_simulation_callbacks']: e for e in events(r) if e['event'] == 'feature-state'}
    assert states[410]['primary_index'] == 0 and states[410]['secondary_index'] == 9
    assert states[472]['camera_index'] == 1 and states[482]['camera_index'] == 0
    assert all(e['destruction_counter'] == 0 for e in states.values())
    assert event(r, 'camera-selector-entry')['index'] == 1  # F2 reselects current view; F1 later changes it
    dispatches = [e for e in events(r) if e['event'] == 'weapon-dispatch']
    assert [(e['completed_simulation_callbacks'], e['callsite'], e['callback']) for e in dispatches] == [(420, 0x420ac9, 0x42f730), (420, 0x420ae8, 0x4305f0), (440, 0x420b61, 0x433dc0), (440, 0x420b80, 0x47d680)]
    assert all(e['weapon'] == states[410]['primary_entity' if i < 2 else 'secondary_entity'] for i, e in enumerate(dispatches))
    returns = [e for e in events(r) if e['event'] == 'weapon-dispatch-return']
    assert [e['address'] for e in returns] == [e['callsite'] + 6 for e in dispatches]
    mission = [e for e in events(r) if e['event'] == 'mission-event']
    assert [(e['completed_simulation_callbacks'], e['type'], e['id']) for e in mission] == [(400, 14, 9991), (400, 14, 3332)]
    keys = [e for e in events(r) if e['event'] == 'child-key-message']
    assert [(e['after_simulation_callbacks'], e['virtual_key'], e['down']) for e in keys if 410 <= e['after_simulation_callbacks'] <= 500] == [(420, 17, True), (428, 17, False), (440, 32, True), (448, 32, False), (470, 113, True), (472, 113, False), (480, 112, True), (482, 112, False)]
    assert all(e['child_pid'] == e['verified_window_owner'] and e['posted'] for e in keys)
    assert event(r, 'original-cleanup-gate')['worker_finished'] == 1
    assert event(r, 'original-cleanup-gate')['shutdown_state'] == 1
    assert [e['address'] for e in events(r) if e['event'] == 'original-cleanup-stage-return'] == list(range(0x479171, 0x47918b, 5))
    assert event(r, 'original-winmain-exit-boundary')['message_wparam'] == 0
    assert event(r, 'process-exit')['code'] == 0 and not event(r, 'process-exit')['probe_termination']
    assert event(r, 'breakpoints-restored')['verified'] and event(r, 'private-key-cleanup')['result'] == 0
    assert not any(e['event'] in ('timeout', 'exception', 'probe-error', 'message-box-boundary', 'volume-write-boundary') for e in events(r))
    assert all(e['x87_control'] == 0x27f for e in events(r) if 'x87_control' in e)
    for f in r['profile_files']:
        assert sha(r['profile_path'] + '/' + f['name']) == f['sha256']
    assert sha(r['compatibility_log']['path']) == r['compatibility_log']['sha256']
    print(json.dumps(dict(passed=True, run=r['run_id'], weapon_dispatches_returned=4, selected_primary_index=0, selected_secondary_index=9, camera_index_change=[1, 0], mission_event_ids=[e['id'] for e in mission], objective_completion_captured=False, destruction_retry_captured=False, natural_exit=True, m3_complete=False)))

if __name__ == '__main__':
    main()
