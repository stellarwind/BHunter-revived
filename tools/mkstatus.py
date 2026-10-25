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


def latest_checkpoint(root, ledger):
    """Use the last admitted owner's recorded validation, never a build-in-progress."""
    for row in reversed(ledger['records']):
        if row.get('historical_variant') or row.get('validation', {}).get('superseded_by') or row.get('implementation') not in {'func', 'bm'} or row.get('integration') not in LINKED:
            continue
        for name in row.get('evidence', []):
            if not name.endswith('/validation.json'):
                continue
            path = root / name
            if not path.is_file():
                continue
            data = json.loads(path.read_text(encoding='utf-8-sig'))
            if not data.get('passed') or not all(k in data for k in ('real_total', 'clean_total', 'translation_units', 'missing_module_controls', 'complete_new_functions')):
                continue
            if data['real_total']['failures'] or data['clean_total']['failures']:
                raise ValueError('Passed checkpoint contains failed checks: ' + name)
            return dict(data, evidence=(path.parent / 'README.md').relative_to(root).as_posix())
    return None


def render(summary, checkpoint=None):
    c = summary['counts']
    total = summary['total']
    return '\n'.join([
        START, '',
        '**Source recovery:** ' + bar(summary['recovered'], total), '',
        '| Restored & linked | In progress | Not started |',
        '|---:|---:|---:|',
        f'| {summary["recovered"]} | {c.get("wip", 0)} | {c.get("not_started", 0)} |', '',
        f'{summary["mapped"]} mapped functions − {summary["excluded"]} verified runtime/helpers = '
        f'{total} potential game/engine functions. Classification remains provisional.', '',
        f'**Playable game target:** {c.get("admitted", 0)}/{total} functions admitted. '
        'Function count is not effort or gameplay completion.', '',
        *([] if checkpoint is None else [
            f'**Latest validated block:** {checkpoint["complete_new_functions"]} complete owners. '
            f'[Block evidence]({checkpoint["evidence"]}).', '',
            f'**Last validated builds:** {checkpoint["translation_units"]:,} translation units · '
            f'{checkpoint["real_total"]["checks"]:,} real-asset checks · '
            f'{checkpoint["clean_total"]["checks"]:,} clean checks · '
            f'{checkpoint["missing_module_controls"]:,} missing-module controls.', '',
        ]), END])

def refresh(root=ROOT, *, check=False, dry_run=False):
    ledger=json.loads((root/'META/function-status.json').read_text(encoding='utf-8-sig'))
    catalog=json.loads((root/ledger['role_catalog']).read_text(encoding='utf-8-sig'))
    summary=summarize(ledger,catalog)
    readme=root/'README.md';text=readme.read_text(encoding='utf-8')
    if text.count(START)!=1 or text.count(END)!=1 or text.index(START)>text.index(END):
        raise ValueError('README needs exactly one correctly ordered status marker pair')
    a=text.index(START);b=text.index(END)+len(END)
    updated=text[:a]+render(summary, latest_checkpoint(root, ledger))+text[b:]
    if check and updated!=text:
        raise ValueError('README status is stale; run mkstatus.cmd')
    if not check and not dry_run and updated!=text:
        readme.write_text(updated,encoding='utf-8',newline='\n')
    return summary, updated!=text


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--check',action='store_true',help='Fail if README differs; do not write')
    ap.add_argument('--dry-run',action='store_true',help='Preview counts without changing files')
    args=ap.parse_args()
    try:
        summary, changed=refresh(check=args.check, dry_run=args.dry_run)
    except (OSError, ValueError, KeyError) as error:
        ap.exit(1, 'STATUS ERROR: '+str(error)+'\n')
    print(('Checked' if args.check else 'Preview' if args.dry_run else 'Updated')+
          ' README: '+json.dumps(summary,sort_keys=True))
    if args.dry_run:
        print('README '+('would change' if changed else 'is current'))


if __name__=='__main__': main()
