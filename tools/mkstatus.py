"""Update README status from explicit per-variant evidence, deduplicated by address."""
import argparse
from collections import Counter
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
START = '<!-- BHUNTER_STATUS_START -->'
END = '<!-- BHUNTER_STATUS_END -->'
EXCLUDED_ROLES = {'library_verified', 'compiler_support', 'import_thunk', 'shared_tail'}
ELIGIBLE_ROLES = {'game', 'engine_candidate', 'unknown'}
STATES = {'not_started', 'wip', 'func', 'bm', 'reused', 'adapter'}
LINKED = {'linked_harness', 'linked_game', 'scenario_exercised', 'release_admitted'}
GAME_LINKED = LINKED - {'linked_harness'}


def address(raw):
    raw = raw.lower().removeprefix('0x')
    if not re.fullmatch(r'[0-9a-f]{1,8}', raw):
        raise ValueError('Invalid original VA: ' + raw)
    return f'{int(raw, 16):08x}'


def summarize(ledger, catalog):
    mapped = {}
    excluded = Counter()
    for row in catalog['records']:
        va = address(row['address'])
        if va in mapped:
            raise ValueError('Duplicate catalog address: ' + va)
        role = row['role']
        if role not in EXCLUDED_ROLES | ELIGIBLE_ROLES:
            raise ValueError('Unknown catalog role: ' + role)
        mapped[va] = role
        if role in EXCLUDED_ROLES:
            excluded[role] += 1
    groups = {va: [] for va, role in mapped.items() if role in ELIGIBLE_ROLES}
    seen_ids = set()
    variants = Counter()
    comparison_addresses = set()
    for row in ledger['records']:
        if row['id'] in seen_ids:
            raise ValueError('Duplicate implementation id: ' + row['id'])
        seen_ids.add(row['id'])
        if row['implementation'] not in STATES:
            raise ValueError('Unknown implementation state: ' + row['implementation'])
        va = address(row['original_va'])
        if va not in mapped:
            raise ValueError('Implementation absent from role catalog: ' + va)
        if row.get('game_admitted') and (row['integration'] not in GAME_LINKED or
                row['implementation'] not in {'bm', 'func', 'adapter', 'reused'}):
            raise ValueError('Invalid game admission claim: ' + row['id'])
        if mapped[va] in EXCLUDED_ROLES:
            if row['role'] in ELIGIBLE_ROLES:
                raise ValueError('Implementation conflicts with excluded catalog role: ' + va)
            continue
        if row.get('historical_variant') or row.get('validation', {}).get('superseded_by'):
            variants['historical_or_superseded'] += 1
        else:
            groups[va].append(row)
            if row['implementation'] == 'bm' and row['integration'] == 'object_only':
                variants['comparison_only'] += 1
                comparison_addresses.add(va)
            else:
                variants['current_other'] += 1
    counts = Counter()
    tracked = set()
    # Historical-only addresses still belong to the map, but are not restored.
    for row in ledger['records']:
        va = address(row['original_va'])
        if va in groups:
            tracked.add(va)
    for rows in groups.values():
        linked = {r['implementation'] for r in rows if r['integration'] in LINKED}
        if 'bm' in linked:
            counts['bm'] += 1
        elif 'func' in linked:
            counts['func'] += 1
        elif any(r['implementation'] == 'wip' for r in rows):
            counts['wip'] += 1
        elif linked & {'adapter', 'reused'}:
            counts['adapted_or_reused'] += 1
        else:
            counts['not_started'] += 1
        if any(r.get('game_admitted') for r in rows):
            counts['admitted'] += 1
    recovered = counts['bm'] + counts['func']
    return dict(mapped=len(mapped), excluded=sum(excluded.values()),
                excluded_by_role=dict(excluded), total=len(groups), tracked=len(tracked),
                variants=len(ledger['records']), variant_counts=dict(variants),
                comparison_addresses=len(comparison_addresses), recovered=recovered,
                remaining=len(groups) - recovered, counts=dict(counts),
                support_implementations=len(ledger.get('support_implementations', [])))


def bar(done, total):
    width = 24
    filled = (width * done // total) if total else 0
    percent = (100 * done / total) if total else 0
    return f'`[{"#" * filled}{"-" * (width - filled)}]` **{percent:.1f}% ({done}/{total})**'


def render(summary):
    c = summary['counts']
    total = summary['total']
    v = summary['variant_counts']
    return '\n'.join([
        START, '',
        '**Restored and linked source functions:** ' + bar(summary['recovered'], total), '',
        '| Mapped | Excluded runtime/helpers | Potential game/engine | Restored & linked | In progress | Not started |',
        '|---:|---:|---:|---:|---:|---:|',
        f'| {summary["mapped"]} | {summary["excluded"]} | {total} | {summary["recovered"]} | {c.get("wip", 0)} | {c.get("not_started", 0)} |', '',
        f'**Remaining:** {summary["remaining"]} potential functions. '
        f'Current linked implementations: {c.get("func", 0)} functional / {c.get("bm", 0)} byte-matched; '
        f'{c.get("adapted_or_reused", 0)} other adapted/reused addresses.', '',
        '**Admitted into a playable game target:** ' + bar(c.get('admitted', 0), total), '',
        f'The denominator uses the **full {summary["mapped"]}-function role catalog**, '
        'including unknown candidates, minus verified libraries, compiler helpers, imports and shared tails. '
        'It is provisional: further classification may exclude more functions. '
        '**Function count is not effort or playable-game completion.**', '',
        f'The ledger tracks {summary["tracked"]} distinct eligible addresses across '
        f'{summary["variants"]} implementation variants: '
        f'{v.get("historical_or_superseded", 0)} historical/superseded and '
        f'{v.get("comparison_only", 0)} object-only byte-match variants are kept separate. '
        'Object-only matches do not replace linked functional evidence or add restored functions. '
        f'{summary["support_implementations"]} source support implementations are counted separately.', '',
        'Sources: [role catalog](META/m1/role-catalog.json), '
        '[implementation ledger](META/function-status.json). '
        'Milestones: [progress and blockers](META/PROGRESS.md).', '', END])

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--check',action='store_true',help='Fail if README differs; do not write')
    args=ap.parse_args()
    ledger=json.loads((ROOT/'META/function-status.json').read_text(encoding='utf-8'))
    catalog=json.loads((ROOT/ledger['role_catalog']).read_text(encoding='utf-8'))
    summary=summarize(ledger,catalog)
    readme=ROOT/'README.md';text=readme.read_text(encoding='utf-8')
    if text.count(START)!=1 or text.count(END)!=1 or text.index(START)>text.index(END):
        raise ValueError('README needs exactly one correctly ordered status marker pair')
    a=text.index(START);b=text.index(END)+len(END)
    updated=text[:a]+render(summary)+text[b:]
    if args.check and updated!=text: raise SystemExit('README status is stale; run mkstatus.cmd')
    if not args.check and updated!=text: readme.write_text(updated,encoding='utf-8',newline='\n')
    print(('Checked' if args.check else 'Updated')+' README: '+json.dumps(summary,sort_keys=True))

if __name__=='__main__': main()
