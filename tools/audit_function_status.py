"""Separate ledger implementation variants without treating them as game completion."""
import collections
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    ledger = json.loads((ROOT / 'META/function-status.json').read_text())
    build = json.loads((ROOT / 'build/m0/result.json').read_text())
    assert build['passed'] and not build['original_code_required']
    groups = collections.defaultdict(list)
    categories = collections.defaultdict(list)
    ids = set()
    for row in ledger['records']:
        assert row['id'] not in ids
        ids.add(row['id'])
        if row.get('original_va'):
            groups[row['original_va']].append(row['id'])
        historical = row.get('historical_variant') or row.get('validation', {}).get('superseded_by')
        if historical:
            category = 'historical_or_superseded'
        elif row['implementation'] in ('not_started', 'wip'):
            category = 'unfinished_active'
        elif row['implementation'] == 'bm' and row['integration'] == 'object_only':
            category = 'completed_comparison_only'
        elif row['implementation'] in ('func', 'bm') and row['integration'] == 'linked_harness':
            category = 'completed_linked_source'
            source = row.get('source')
            assert source in build['sources'], (row['id'], source)
            assert hashlib.sha256((ROOT / source).read_bytes()).hexdigest() == build['sources'][source]
        else:
            category = 'other_explicit_state'
        categories[category].append(dict(id=row['id'], address=row.get('original_va'),
                                        source=row.get('source'), implementation=row['implementation'],
                                        integration=row['integration'], game_admitted=row.get('game_admitted', False)))
    duplicate_addresses = {a: rows for a, rows in groups.items() if len(rows) > 1}
    report = dict(ledger_sha256=hashlib.sha256((ROOT / 'META/function-status.json').read_bytes()).hexdigest(),
                  build_source_hashes_verified=True, total_variants=len(ids), unique_original_addresses=len(groups),
                  categories=dict(categories), category_counts={k: len(v) for k, v in categories.items()},
                  duplicate_addresses=duplicate_addresses,
                  support_implementations=len(ledger.get('support_implementations', [])),
                  original_backed_diagnostics=len(ledger.get('reference_diagnostics', [])),
                  game_admitted_variants=sum(bool(r.get('game_admitted')) for r in ledger['records']),
                  limits=['Completed variants are not deduplicated source-game completion.',
                          'Historical byte evidence is not transferred to current source variants.',
                          'An incomplete reader subset remains unfinished even when its adapter passes tests.',
                          'The ledger is not an exhaustive inventory of all M4 dependencies.'])
    target = ROOT / 'META/m4/ledger-audit.json'
    target.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('total_variants', 'unique_original_addresses', 'category_counts', 'game_admitted_variants')}))


if __name__ == '__main__':
    main()
