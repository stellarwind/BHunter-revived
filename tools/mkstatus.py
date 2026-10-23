"""Update README status from explicit per-variant evidence, deduplicated by address."""
import argparse
from collections import Counter
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
START = '<!-- BHUNTER_STATUS_START -->'
END = '<!-- BHUNTER_STATUS_END -->'
ROLES = {'game', 'engine_candidate'}
STATES = {'not_started','wip','func','bm','reused','adapter'}

def summarize(ledger):
    groups = {}
    seen_ids = set()
    for row in ledger['records']:
        if row['id'] in seen_ids: raise ValueError('Duplicate implementation id: '+row['id'])
        seen_ids.add(row['id'])
        if row['implementation'] not in STATES: raise ValueError('Unknown implementation state: '+row['implementation'])
        if row['role'] not in ROLES: continue
        raw = row['original_va'].lower().removeprefix('0x')
        if not re.fullmatch(r'[0-9a-f]{1,8}',raw): raise ValueError('Invalid original VA: '+raw)
        address = f'{int(raw,16):08x}'
        groups.setdefault(address,[]).append(row)
    counts = Counter()
    for rows in groups.values():
        states={r['implementation'] for r in rows}
        status=next(s for s in ['bm','func','adapter','reused','wip','not_started'] if s in states)
        counts[status]+=1
        if any(r.get('game_admitted') for r in rows):
            for r in rows:
                if r.get('game_admitted') and (r['integration'] not in {'linked_game','scenario_exercised','release_admitted'} or r['implementation'] not in {'bm','func','adapter','reused'}):
                    raise ValueError('Invalid game admission claim: '+r['id'])
            counts['admitted']+=1
    return dict(total=len(groups),variants=len(ledger['records']),counts=dict(counts))

def bar(done,total):
    width=24
    filled=(width*done//total) if total else 0
    percent=(100*done/total) if total else 0
    return f'`[{"#"*filled}{"-"*(width-filled)}]` **{percent:.0f}% ({done}/{total})**'

def render(summary):
    c=summary['counts'];total=summary['total']
    recovered=c.get('bm',0)+c.get('func',0)
    admitted=c.get('admitted',0)
    lines=[START,'', '**Tracked recovery evidence — catalogued functions only:** '+bar(recovered,total),'',
           '| Byte-matched | Functional | In progress | Not started | Adapters / reused |',
           '|---:|---:|---:|---:|---:|',
           f'| {c.get("bm",0)} | {c.get("func",0)} | {c.get("wip",0)} | {c.get("not_started",0)} | {c.get("adapter",0)} / {c.get("reused",0)} |','',
           '**Admitted into the game target:** '+bar(admitted,total),'',
           f'This bar covers **{total} catalogued game/engine function addresses**, deduplicated from {summary["variants"]} implementation records. It is **not overall game completion**. A byte-matched object can remain unintegrated. Verified CRT/library/compiler-support records are excluded; adapters/reused dependencies are shown separately from `bm`/`func`.','',
           'Source: [implementation ledger](META/function-status.json). Milestones: [progress and blockers](META/PROGRESS.md).','',END]
    return '\n'.join(lines)

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--check',action='store_true',help='Fail if README differs; do not write')
    args=ap.parse_args()
    ledger=json.loads((ROOT/'META/function-status.json').read_text(encoding='utf-8'))
    summary=summarize(ledger)
    readme=ROOT/'README.md';text=readme.read_text(encoding='utf-8')
    if text.count(START)!=1 or text.count(END)!=1 or text.index(START)>text.index(END):
        raise ValueError('README needs exactly one correctly ordered status marker pair')
    a=text.index(START);b=text.index(END)+len(END)
    updated=text[:a]+render(summary)+text[b:]
    if args.check and updated!=text: raise SystemExit('README status is stale; run mkstatus.cmd')
    if not args.check and updated!=text: readme.write_text(updated,encoding='utf-8',newline='\n')
    print(('Checked' if args.check else 'Updated')+' README: '+json.dumps(summary,sort_keys=True))

if __name__=='__main__': main()
