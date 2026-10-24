"""Audit original DirectSound API calls; never infer audible output."""
import json
from check_m3_startup_evidence import ROOT, read, sha, events, event

def main():
    r = read('META/m3/startup/7283583a444c.json')
    assert r['passed'] and r['sound_trace'] and r['observe_callbacks'] == 300
    assert r['original_code_executes'] and r['original_file_patches'] == 0
    assert not r['input_injection_used'] and not r['desktop_input_sent'] and not r['visual_ui_tooling_used']
    assert r['registry_before'] == r['registry_after'] and r['private_key_absent_after']
    assert r['protected_windows_files_before'] == r['protected_windows_files_after']
    for p, h in r['original_hashes'].items():
        assert sha(p) == h
    for p, h in r['source_hashes'].items():
        assert sha(r['source_snapshots'][p]) == h
    assert all(c['exit_code'] == 0 for c in r['commands'])
    assert event(r, 'source-check-return')['eax'] == 1
    assert event(r, 'bootstrap-return')['eax'] == 1
    assert event(r, 'first-surface-flip-result')['hresult'] == 0
    assert event(r, 'observation-boundary')['completed_simulation_callbacks'] == 300
    calls = [e for e in events(r) if e['event'] == 'directsound-play-call']
    returns = [e for e in events(r) if e['event'] == 'directsound-play-return']
    assert [(e['callsite'], e['flags']) for e in calls] == [(0x47b5c4, 1), (0x47ca09, 0)]
    assert all(e['reserved'] == [0, 0] and e['buffer'] and e['api_target'] for e in calls)
    assert calls[0]['buffer'] != calls[1]['buffer']
    assert calls[0]['completed_simulation_callbacks'] == 0 and calls[1]['completed_simulation_callbacks'] == 299
    assert [e['address'] for e in returns] == [e['callsite'] + 3 for e in calls]
    assert all(e['hresult'] == 0 for e in returns)
    status = [e for e in events(r) if e['event'] == 'directsound-buffer-status']
    assert not status  # Do not manufacture status or full playback-lifetime evidence.
    assert event(r, 'breakpoints-restored')['verified']
    assert event(r, 'process-exit')['probe_termination']
    assert event(r, 'private-key-cleanup')['result'] == 0
    assert not any(e['event'] in ('timeout', 'exception', 'probe-error', 'message-box-boundary', 'volume-write-boundary', 'child-key-message') for e in events(r))
    assert all(e['x87_control'] == 0x27f for e in events(r) if 'x87_control' in e)
    for f in r['profile_files']:
        assert sha(r['profile_path'] + '/' + f['name']) == f['sha256']
    assert sha(r['compatibility_log']['path']) == r['compatibility_log']['sha256']
    print(json.dumps(dict(passed=True, run=r['run_id'], primary_play_hresult=0, secondary_oneshot_play_hresult=0, buffer_status_captured=False, audible_output_verified=False, cd_playback_verified=False, gameplay_input=False, m3_complete=False)))

if __name__ == '__main__':
    main()
