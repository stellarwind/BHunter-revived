"""Summarize passive source-port frame samples; never starts or controls a game."""
import argparse
import csv
import json
import math
from pathlib import Path


def percentile(values, fraction):
    ordered = sorted(values)
    at = (len(ordered) - 1) * fraction
    lo = int(at)
    hi = min(lo + 1, len(ordered) - 1)
    return ordered[lo] + (ordered[hi] - ordered[lo]) * (at - lo)


def summarize(rows):
    groups = {name: [] for name in ('mission_all', 'idle', 'projectiles', 'collision')}
    for row in rows:
        if int(row['mode']) == 0 or int(row['paused']) or float(row['interval_ms']) <= 0:
            continue
        if not all(math.isfinite(float(row[key])) and float(row[key]) >= 0
                   for key in ('interval_ms', 'simulation_ms', 'render_ms', 'present_ms')):
            raise ValueError('Invalid timing sample')
        groups['mission_all'].append(row)
        group = 'collision' if int(row['contacts']) else 'projectiles' if int(row['projectiles']) else 'idle'
        groups[group].append(row)
    result = {}
    for name, samples in groups.items():
        if not samples:
            result[name] = dict(samples=0)
            continue
        intervals = [float(row['interval_ms']) for row in samples]
        produced = [float(row['simulation_ms']) for row in samples if int(row['produced'])]
        result[name] = dict(samples=len(samples), simulation_samples=len(produced),
                            mean_ms=sum(intervals) / len(intervals),
                            median_ms=percentile(intervals, .5),
                            p95_ms=percentile(intervals, .95), p99_ms=percentile(intervals, .99),
                            worst_ms=max(intervals), frames_over_33ms=sum(v > 33.333333 for v in intervals),
                            simulation_p95_ms=percentile(produced, .95) if produced else None,
                            render_p95_ms=percentile([float(row['render_ms']) for row in samples], .95),
                            present_p95_ms=percentile([float(row['present_ms']) for row in samples], .95))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('captures', nargs='+', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    result = dict(captures={}, limits=[
        'Source CPU wall-clock timings around rendering and Flip/Blt; not GPU timestamps or externally measured display scanout.',
        'Categories use live entities registered with the projectile lifetime query and collision counters; other effect families may not be classified as projectiles.',
        'Menu, paused and focus-resume samples excluded; manual scenarios must match for cross-build comparison.',
        'Profile buffer is written on orderly exit; a process crash can lose samples. No original-game measurement is inferred.'
    ])
    for path in args.captures:
        with path.open(newline='', encoding='utf-8') as stream:
            result['captures'][str(path.resolve())] = summarize(list(csv.DictReader(stream)))
    if args.output:
        args.output.write_text(json.dumps(result, indent=2) + '\n')
    print('Capture / category                   samples   median     p95     p99   worst ms')
    for path, groups in result['captures'].items():
        print(Path(path).name)
        for name, data in groups.items():
            if data['samples']:
                print(f"  {name:34} {data['samples']:7} {data['median_ms']:8.2f} {data['p95_ms']:7.2f} {data['p99_ms']:7.2f} {data['worst_ms']:8.2f}")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
