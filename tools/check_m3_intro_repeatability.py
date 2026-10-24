"""Compare independently executed introductory prefixes; no game replay."""
import argparse
import hashlib
import json
from check_m3_startup_evidence import ROOT, read, sha, events, event

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--write-report', action='store_true')
    args = ap.parse_args()
    names = ['14bdee1a5329', '80df3c4686e1']
    records = [read('META/m3/startup/' + n + '.json') for n in names]
    fields = ['phase','completed_simulation_callbacks','mode_raw','pause_raw','failure_raw','rng_state','accelerate_key','accelerate_key_state','position_words','velocity_words','control_words']
    streams = []
    for r in records:
        assert r['passed'] and r['renderer_preset']['initial_cfg_sha256'] == records[0]['renderer_preset']['initial_cfg_sha256']
        assert all(c['exit_code'] == 0 for c in r['commands'])
        for p, h in r['source_hashes'].items():
            assert sha(r['source_snapshots'][p]) == h
        for p, h in r['original_hashes'].items():
            assert sha(p) == h
        assert event(r, 'initial-rng-state')['state'] == 1
        assert all(e['x87_control'] == 0x27f for e in events(r) if 'x87_control' in e)
        streams.append(dict(player_states=[{k:e[k] for k in fields} for e in events(r) if e['event']=='player-state' and e['completed_simulation_callbacks']<=544],
            keys=[{k:e[k] for k in ['after_simulation_callbacks','virtual_key','down']} for e in events(r) if e['event']=='child-key-message' and e['after_simulation_callbacks']<544],
            flight_observations=[e for e in events(r) if e['event']=='flight-observation' and e['completed_simulation_callbacks']<=542]))
    assert records[0]['source_hashes'] != records[1]['source_hashes']
    assert streams[0] == streams[1]
    assert [len(streams[0][k]) for k in ['player_states','keys','flight_observations']] == [20,37,23]
    encoded = json.dumps(streams[0], sort_keys=True, separators=(',',':')).encode()
    report = dict(scope='Observed frame-indexed common introductory prefix in two independently built/executed variants', runs=names, evidence_sha256={n:sha('META/m3/startup/'+n+'.json') for n in names}, source_variants_distinct=True, initial_rng_state=1, x87_control=0x27f, exact_player_samples=20, matched_key_events=37, matched_printed_flight_observations=23, canonical_trace_sha256=hashlib.sha256(encoded).hexdigest(), trace=streams[0], limits=['Raw word equality applies to selected player samples, not all world memory','Flight observation floats are printed values, not a byte-match claim','Wall-clock time, background threads and physical cursor movement are not pinned','This comparison is not instruction matching or source-game implementation admission'], full_world_determinism_validated=False, m3_complete=False)
    path = ROOT/'META/m3/intro-prefix-trace.json'
    if args.write_report:
        path.write_text(json.dumps(report,indent=2)+'\n')
    else:
        assert read('META/m3/intro-prefix-trace.json') == report
    print(json.dumps(dict(passed=True, runs=names, exact_player_samples=20, matched_key_events=37, matched_flight_observations=23, full_world_determinism_validated=False)))

if __name__ == '__main__':
    main()
