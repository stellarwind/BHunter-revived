"""Static whole-TU COFF proof for explicitly observed AI variants. Executes no code."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'analysis/astra-review'))
from targeted_probes import parse_coff


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('production', type=Path)
    parser.add_argument('diagnostic', type=Path)
    parser.add_argument('--aliases-from', type=Path, action='append', default=[])
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    aliases = {}
    for path in args.aliases_from:
        for source, target in re.findall(r'/D([a-z_][a-z_0-9]*)=(oracle_[a-z_]+)', path.read_text()):
            aliases['_' + source] = '_' + target
    ab, bb = args.production.read_bytes(), args.diagnostic.read_bytes()
    asym, asec = parse_coff(ab)
    bsym, bsec = parse_coff(bb)
    af = {v['name']: v for v in asym.values() if v['section'] > 0 and v['type'] == 32}
    bf = {v['name']: v for v in bsym.values() if v['section'] > 0 and v['type'] == 32}
    assert af and af.keys() == bf.keys(), 'Function sets differ'
    seen = set()

    def equivalent(u, v):
        un, vn = u['name'], v['name']
        if aliases.get(un, un) != vn:
            assert ((un.startswith('$') and vn.startswith('$')) or
                    ('$S' in un and un.split('$S')[0] == vn.split('$S')[0])), (un, vn)
        if u['section'] <= 0 or v['section'] <= 0:
            assert u['section'] == v['section'], 'Definition boundary differs'
            return
        if u['type'] == 32 or v['type'] == 32:
            assert u['type'] == v['type'] and un == vn
            return  # Every defined function is checked by the outer loop.
        key = (u['section'], u['value'], v['section'], v['value'])
        if key in seen:
            return
        seen.add(key)
        a, b = asec[u['section']], bsec[v['section']]
        assert a['data'][u['value']:] == b['data'][v['value']:], (un, vn, 'initialized data')
        ar = [r for r in a['relocations'] if r['offset'] >= u['value']]
        br = [r for r in b['relocations'] if r['offset'] >= v['value']]
        assert len(ar) == len(br)
        for x, y in zip(ar, br):
            assert (x['offset'] - u['value'], x['kind']) == (y['offset'] - v['value'], y['kind'])
            equivalent(x['symbol'], y['symbol'])

    functions = []
    for name, a in af.items():
        b = bf[name]
        sa, sb = asec[a['section']], bsec[b['section']]
        body = sa['data'][a['value']:]
        assert body == sb['data'][b['value']:], (name, 'instruction data')
        assert len(sa['relocations']) == len(sb['relocations'])
        bindings = []
        for x, y in zip(sa['relocations'], sb['relocations']):
            offset = x['offset'] - a['value']
            assert (offset, x['kind']) == (y['offset'] - b['value'], y['kind'])
            equivalent(x['symbol'], y['symbol'])
            if x['symbol']['name'] != y['symbol']['name']:
                bindings.append(dict(offset=offset, kind=x['kind'], production=x['symbol']['name'],
                                     diagnostic=y['symbol']['name'], verified=True))
        functions.append(dict(symbol=name, whole_coff_section_equal=True, section_bytes=len(body),
                              instruction_data_sha256=hashlib.sha256(body).hexdigest(), bindings=bindings))
    result = dict(comparison_kind='production_to_diagnostic_only', original_byte_match_claim=False,
                  production=str(args.production), production_sha256=hashlib.sha256(ab).hexdigest(),
                  diagnostic=str(args.diagnostic), diagnostic_sha256=hashlib.sha256(bb).hexdigest(),
                  alias_sources=[str(p) for p in args.aliases_from], functions=functions)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(f'{len(functions)} complete function sections and referenced initialized data agree')


if __name__ == '__main__':
    main()
