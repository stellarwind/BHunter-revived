"""Audit an original startup graph and derive a portal route without gameplay."""
import collections
import heapq
import json
import math
import struct
from check_m3_startup_evidence import ROOT, read, sha, event, events


def f32(word):
    return struct.unpack('<f', struct.pack('<I', word))[0]


def signed(word):
    return struct.unpack('<i', struct.pack('<I', word))[0]


def main():
    source = 'META/m3/startup/2e901f78ca70.json'
    r = read(source)
    assert r['passed'] and r['navigation_snapshot'] and r['through_bootstrap']
    assert not r['through_first_frame'] and not r['input_injection_used']
    assert not r['play_trace'] and not r['visual_ui_tooling_used'] and not r['desktop_input_sent']
    assert not r['original_file_patches'] and not r['m3_complete']
    assert r['registry_before'] == r['registry_after'] and r['private_key_absent_after']
    assert r['protected_windows_files_before'] == r['protected_windows_files_after']
    assert [c['exit_code'] for c in r['commands']] == [0, 0, 0]
    for p, h in r['source_hashes'].items():
        assert sha(r['source_snapshots'][p]) == h
    for p, h in r['original_hashes'].items():
        assert sha(p) == h
    contract = read('META/m3/navigation-contract.json')
    assert not contract['source_game_admission']
    for p, h in contract['evidence_sha256'].items():
        assert sha(p) == h
    es = events(r)
    assert event(r, 'bootstrap-return')['eax'] == 1
    assert event(r, 'breakpoints-restored')['verified'] and event(r, 'private-key-cleanup')['result'] == 0
    assert event(r, 'process-exit')['probe_termination']
    assert not any(e['event'] in ('child-key-message', 'first-simulation-entry', 'probe-error', 'exception', 'timeout') for e in es)
    header = event(r, 'navigation-graph-header')
    assert (header['nodes'], header['junctions']) == (1454, 283)
    nodes = {e['index']: e for e in es if e['event'] == 'navigation-node'}
    assert len(nodes) == header['nodes'] and set(nodes) == set(range(1454))
    pointers = {n['entity']: i for i, n in nodes.items()}
    assert len(pointers) == len(nodes)
    ports = collections.defaultdict(list)
    records = [e for e in es if e['event'] == 'navigation-port']
    assert len(records) == 3739
    for p in records:
        ports[p['node']].append(p)
    for i, n in nodes.items():
        assert sorted(p['slot'] for p in ports[i]) == list(range(n['ports']))
        assert all(math.isfinite(v) for v in n['position'] + n['bounds'])
        assert all(n['bounds'][j] <= n['bounds'][j + 3] for j in range(3))
    connected = []
    for p in records:
        w = p['raw_words']
        assert len(w) == 15 and w[0] in (0, 1, 2, 4, 5, 6) and w[14] == p['neighbor']
        if not p['neighbor']:
            assert p['neighbor_index'] == 0xffffffff
            continue
        assert pointers[p['neighbor']] == p['neighbor_index']
        opposite = [q for q in ports[p['neighbor_index']] if q['neighbor_index'] == p['node'] and q['raw_words'][0] == (w[0] ^ 4) and q['raw_words'][1] == w[1]]
        assert len(opposite) == 1
        a, b = list(map(f32, w[2:6])), list(map(f32, opposite[0]['raw_words'][2:6]))
        overlap = list(map(f32, w[6:10]))
        expected = [max(a[0], b[0]), min(a[1], b[1]), max(a[2], b[2]), min(a[3], b[3])]
        assert all(abs(x - y) < 0.001 for x, y in zip(overlap, expected))
        assert overlap[0] < overlap[1] and overlap[2] < overlap[3]
        connected.append(p)
    assert len(connected) == 3120
    degrees = {i: sum(bool(p['neighbor']) for p in ports[i]) for i in nodes}
    assert all((degrees[i] != 2) == (i < header['junctions']) for i in nodes)
    matrix_path = r['profile_path'] + '/navigation-matrix.bin'
    matrix = (ROOT / matrix_path).read_bytes()
    assert len(matrix) == 283 * 283 * 8 == event(r, 'navigation-matrix-saved')['bytes']
    assert event(r, 'navigation-matrix-saved')['passed']
    index = read('analysis/assets-index.json')
    archive = (ROOT / 'GAME_DATA/Bhunter.bin').read_bytes()
    level = next(e for e in index if e['name'] == 'TEST.LEV')
    level_data = archive[level['offset']:level['end']]
    placement_count = struct.unpack_from('<I', level_data)[0]
    placements = list(struct.iter_unpack('<16siiiI', level_data[4:4 + placement_count * 32]))
    assert len({n['placement'] for n in nodes.values()}) == len(nodes)
    for n in nodes.values():
        placement = placements[n['placement']]
        assert n['position'] == list(placement[1:4])
    pth = next(e for e in index if e['name'] == 'TEST.PTH')
    original = archive[pth['offset']:pth['end']]
    assert struct.unpack_from('<I', original)[0] == 283 and matrix == original[4:]
    owners = {e['placement']: e for e in es if e['event'] == 'navigation-placement-owner'}
    assert set(owners) == {4939, 5055, 5064, 4827}
    for e in owners.values():
        assert e['owner_in_graph'] and nodes[e['node_index']]['entity'] == e['owner']
    start, goal = owners[5064]['node_index'], owners[4827]['node_index']
    assert (start, goal) == (1296, 220)
    centers = {i: tuple((n['bounds'][j] + n['bounds'][j + 3]) / 2 for j in range(3)) for i, n in nodes.items()}
    distance, previous, queue = {start: 0.0}, {}, [(0.0, start)]
    while queue:
        cost, i = heapq.heappop(queue)
        if cost != distance[i]:
            continue
        if i == goal:
            break
        for p in ports[i]:
            if not p['neighbor']:
                continue
            j = p['neighbor_index']
            trial = cost + math.dist(centers[i], centers[j])
            if trial < distance.get(j, math.inf):
                distance[j], previous[j] = trial, (i, p)
                heapq.heappush(queue, (trial, j))
    assert goal in distance
    route, i = [], goal
    while i != start:
        a, p = previous[i]
        route.append((a, p, i))
        i = a
    route.reverse()
    assert len(route) == 15
    steps, decisions = [], []
    for a, p, b in route:
        w = p['raw_words']
        overlap = list(map(f32, w[6:10]))
        axis = {0: 0, 4: 0, 1: 1, 5: 1, 2: 2, 6: 2}[w[0]]
        other = [j for j in range(3) if j != axis]
        point = [0.0] * 3
        point[axis] = signed(w[1])
        point[other[0]], point[other[1]] = (overlap[0] + overlap[1]) / 2, (overlap[2] + overlap[3]) / 2
        for owner in (a, b):
            bounds = nodes[owner]['bounds']
            assert all(bounds[j] - 0.001 <= point[j] <= bounds[j + 3] + 0.001 for j in range(3))
        step = dict(source=a, source_placement=nodes[a]['placement'], exit_slot=p['slot'], destination=b,
                    destination_placement=nodes[b]['placement'], portal_center=point,
                    opening_sizes=[overlap[1] - overlap[0], overlap[3] - overlap[2]])
        if a < 283:
            choice, length = struct.unpack_from('<if', matrix, (goal * 283 + a) * 8)
            remaining = sum(math.dist(centers[x], centers[y]) for x, _, y in route[len(steps):])
            assert choice == p['slot'] and abs(length - remaining) < 0.002
            decisions.append(dict(node=a, exit_slot=choice, original_distance=length))
        steps.append(step)
    assert len(decisions) == 4
    for f in r['profile_files']:
        assert sha(r['profile_path'] + '/' + f['name']) == f['sha256']
    assert sha(r['compatibility_log']['path']) == r['compatibility_log']['sha256']
    candidate = dict(scope='Graph-connected candidate route, not swept ship clearance or successful flight',
                     reference_run=r['run_id'], evidence_sha256=sha(source), start_placement=5064,
                     target_launcher=4827, start_node=start, goal_node=goal,
                     center_distance=distance[goal], steps=steps, original_matrix_decisions=decisions,
                     goal_bounds_center=centers[goal], runtime_route_flown=False,
                     actual_target_pursuit_validated=False, source_game_admission=False)
    (ROOT / 'META/m3/navigation-route.json').write_text(json.dumps(candidate, indent=2) + '\n')
    validation = dict(passed=True, run=r['run_id'], nodes=len(nodes), junctions=283, ports=len(records),
                      reciprocal_connections=len(connected), runtime_matrix_matches_archive=True,
                      candidate_route_edges=len(route), original_matrix_decisions=len(decisions),
                      input_sent=False, gameplay_sequence_replayed=False, runtime_route_flown=False, m3_complete=False)
    (ROOT / 'META/m3/navigation-validation.json').write_text(json.dumps(validation, indent=2) + '\n')
    print(json.dumps(validation))


if __name__ == '__main__':
    main()
