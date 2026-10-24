"""Generate a size map from reviewed original bodies and the restoration ledger."""
import csv
import io
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(path):
    return json.loads((ROOT / path).read_text(encoding='utf-8-sig'))

def generate():
    with (ROOT / 'analysis/function-map.tsv').open(encoding='utf-8-sig', newline='') as stream:
        mapped = list(csv.DictReader(stream, delimiter='\t'))
    catalog = {r['address']: r for r in read('META/m1/role-catalog.json')['records']}
    ledger = read('META/function-status.json')
    active = {}
    for row in ledger['records']:
        address = row.get('original_va')
        if address and not row.get('historical_variant') and not row.get('validation', {}).get('superseded_by'):
            active.setdefault(address, []).append(row)
    def priority(row):
        if row['implementation'] in ('func', 'bm') and row['integration'] == 'linked_harness':
            return 4
        if row['implementation'] == 'bm':
            return 3
        return 2 if row['implementation'] == 'wip' else 1
    rows = []
    for original in mapped:
        address = original['address']
        role = catalog[address]
        size = int(original['body_bytes'])
        assert size == sum(int(end, 16) - int(start, 16) + 1
                           for start, end in (span.split('-') for span in role['body_ranges']))
        variants = active.get(address, [])
        selected = max(variants, key=priority) if variants else None
        rows.append(dict(address='0x'+address, bytes=size, instructions=int(original['instructions']),
                         mapped_name=original['name'],
                         source_name=selected['function'] if selected else '',
                         implementation=selected['implementation'] if selected else 'not tracked',
                         integration=selected['integration'] if selected else '',
                         catalog_role=role['role'], body_ranges=';'.join(role['body_ranges']),
                         source=selected.get('source', '') if selected else ''))
    rows.sort(key=lambda r: int(r['address'], 16))
    assert len(rows) == len({r['address'] for r in rows}) == len(catalog)
    largest = sorted(rows, key=lambda r: (-r['bytes'], r['address']))
    def escape(text):
        return str(text).replace('|', '\\|').replace('\n', ' ')
    def table(items):
        result = ['| Address | Bytes | Instructions | Function / source name | Implementation | Catalog role |',
                  '|---|---:|---:|---|---|---|']
        for row in items:
            name = row['source_name'] or row['mapped_name']
            state = row['implementation']
            if row['integration']:
                state += ' / ' + row['integration']
            result.append('| '+ ' | '.join(escape(x) for x in
                          (row['address'], row['bytes'], row['instructions'], name, state, row['catalog_role']))+' |')
        return result
    total = sum(r['bytes'] for r in rows)
    lines = ['# BHunter function-size map', '', 'Snapshot: 2026-10-24.', '',
             f'{len(rows):,} mapped original function entries; {total:,} bytes '
             f'({total/1024:.2f} KiB) summed across their mapped bodies.', '',
             'Sizes are original x86 machine-code body bytes, not C source sizes or estimates of remaining effort. '
             'They exclude padding between functions. Shared/overlapping bodies, if present, contribute to each entry’s size; '
             'the sum is not the executable file size. Instructions are decoded instruction counts.', '',
             'Names and body sizes come from `analysis/function-map.tsv`; ranges and roles come from '
             '`META/m1/role-catalog.json`. Source names and implementation/integration states come from '
             '`META/function-status.json`, preferring the current linked functional variant. '
             'Catalog roles and implementation states are independent; older unknown provenance may coexist with restored source. '
             '“Not tracked” means no active ledger variant, not proof that an entry is game-owned or needs restoration. '
             'CRT, libraries, imports and compiler helpers remain included and labelled. '
             'No source-built game admission or completion percentage is inferred.', '',
             '## Largest 30 mapped functions', ''] + table(largest[:30]) + [
             '', '## Complete map in address order', ''] + table(rows) + [
             '', 'Regenerate with `python tools/mk_function_size_map.py`. '
             'Machine-readable companion: `META/function-size-map.tsv`.', '']
    (ROOT / 'META/function-size-map.md').write_bytes('\n'.join(lines).encode('utf-8'))
    output = io.StringIO(newline='')
    writer = csv.DictWriter(output, fieldnames=list(rows[0]), delimiter='\t', lineterminator='\n')
    writer.writeheader(); writer.writerows(rows)
    data = output.getvalue()
    assert len(list(csv.DictReader(io.StringIO(data), delimiter='\t'))) == len(rows)
    (ROOT / 'META/function-size-map.tsv').write_bytes(data.encode('utf-8'))
    print(json.dumps(dict(functions=len(rows), summed_body_bytes=total,
                          largest=largest[0]['address'], largest_bytes=largest[0]['bytes'],
                          markdown='META/function-size-map.md', tsv='META/function-size-map.tsv')))

if __name__ == '__main__':
    generate()
