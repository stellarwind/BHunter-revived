"""Read-only audit of archived original player, collision and natural-exit traces."""
import json
import math
import struct
from check_m3_startup_evidence import ROOT, read, sha, events, event

RUNS = ['8ac1ede6c68e', 'a43f71e721d1', '94a81f9b2de5', '0187240c133a', 'dfa3b4981226']

def value(word):
    return struct.unpack('<f', struct.pack('<I', word))[0]

def state(r, count):
    return next(e for e in events(r) if e['event'] == 'player-state' and e['phase'] == 'simulation-return' and e['completed_simulation_callbacks'] == count)

def main():
    records = {n: read('META/m3/startup/' + n + '.json') for n in RUNS}
    refs = {v['path']: v['sha256'] for v in read('META/reference-manifest.json')['files'] if v['root'] == 'repo'}
    for p, h in refs.items():
        assert sha(p) == h, p
    for n, r in records.items():
        assert r['passed'] and not r['m3_complete'] and r['requested_menu_path_observed']
        assert r['original_code_executes'] and r['original_file_patches'] == 0 and r['original_hashes'] == refs
        assert r['registry_before'] == r['registry_after'] and r['private_key_absent_after']
        assert r['protected_windows_files_before'] == r['protected_windows_files_after']
        assert not r['desktop_input_sent'] and not r['visual_ui_tooling_used']
        assert r['relative_cursor_adapter'] and r['crt_initial_heap_mib_adapter'] == 128
        for p, h in r['source_hashes'].items():
            assert sha(r['source_snapshots'][p]) == h, (n, p)
        for f in r['profile_files']:
            p = r['profile_path'] + '/' + f['name']
            assert sha(p) == f['sha256'] and (ROOT / p).stat().st_size == f['bytes']
        assert sha(r['compatibility_log']['path']) == r['compatibility_log']['sha256']
        assert event(r, 'episode-action-entry')['episode_index'] == 0
        assert event(r, 'player-reset-entry')['spawn_index'] == 21
        assert event(r, 'player-physics-entry')['entity_argument'] == state(r, 450)['player']
        assert event(r, 'player-physics-entry')['delta_word'] == 0x3d088889
        assert event(r, 'original-return-to-menu-entry')['mode_raw'] == 1
        assert event(r, 'original-return-to-menu-return')['mode_raw'] == 0
        checkpoint = next(e for e in events(r) if e['event'] == 'player-state' and e['completed_simulation_callbacks'] == 552)
        assert checkpoint['pause_raw'] == 1
        assert event(r, 'original-quit-action')['completed_simulation_callbacks'] == 660
        gate = event(r, 'original-cleanup-gate')
        assert gate['shutdown_state'] == gate['worker_finished'] == 1
        assert [e['address'] for e in events(r) if e['event'] == 'original-cleanup-stage-return'] == list(range(0x479171, 0x47918b, 5))
        assert event(r, 'original-winmain-exit-boundary')['message_wparam'] == 0
        assert event(r, 'breakpoints-restored')['verified']
        assert event(r, 'process-exit')['code'] == 0 and not event(r, 'process-exit')['probe_termination']
        assert event(r, 'private-key-cleanup')['result'] == 0 and event(r, 'probe-complete')['passed']
        assert not any(e['event'] in ('timeout', 'exception', 'volume-write-boundary', 'message-box-boundary') for e in events(r))
        for e in events(r):
            if 'x87_control' in e:
                assert e['x87_control'] == 0x27f
            if e['event'] == 'child-key-message':
                assert e['child_pid'] == e['verified_window_owner'] and e['posted']
            if e['event'] == 'player-state':
                assert all(math.isfinite(value(w)) for k in ('position_words', 'velocity_words', 'control_words') for w in e[k])
    motion, idle = records[RUNS[0]], records[RUNS[1]]
    assert motion['source_hashes'] == idle['source_hashes']
    displacement = {}
    for name, r, held in [('motion', motion, 1), ('idle', idle, 0)]:
        a, b = state(r, 450), state(r, 510)
        assert b['accelerate_key'] == 65 and b['accelerate_key_state'] == held
        assert value(b['control_words'][2]) == held
        assert state(r, 540)['accelerate_key_state'] == 0
        displacement[name] = math.dist([value(w) for w in a['position_words']], [value(w) for w in b['position_words']])
    assert displacement['motion'] > 80 and displacement['idle'] < 1
    final = records[RUNS[-1]]
    # Each executed variant is tied to its own immutable source snapshots above.
    hit = next(e for e in events(final) if e['event'] == 'collision-detector-result' and e['result'])
    assert hit['physics_entity'] == hit['player'] == state(final, 450)['player']
    assert hit['result'] == 1 and hit['completed_simulation_callbacks'] == 538
    assert 0 <= value(hit['fraction_word']) <= 1
    normal = [value(w) for w in hit['normal_words']]
    assert abs(math.dist(normal, [0, 0, 0]) - 1) < 0.00001
    assert value(state(final, 510)['velocity_words'][0]) < 0 < value(state(final, 540)['velocity_words'][0])
    print(json.dumps(dict(passed=True, runs=RUNS, same_source_control_pair=RUNS[:2], displacement=displacement, collision_callback=538, collision_fraction=value(hit['fraction_word']), collision_normal=normal, natural_exits=5, visual_tools_used=False, m3_complete=False)))

if __name__ == '__main__':
    main()
