"""Extract saved world bounds and model grid inputs; no game execution."""
import hashlib
import json
import re
import struct
from pathlib import Path
from geometry_fixtures import ROOT


def main():
    source = ROOT / 'META/m3/startup/2e901f78ca70.json'
    record = json.loads(source.read_text())
    events = record.get('events') or [json.loads(line) for c in record['commands'] if c['label'] == 'startup'
                                    for line in c['output'].splitlines() if line.startswith('{')]
    nodes = sorted((e for e in events if e['event'] == 'navigation-node'), key=lambda n: n['index'])
    owners = [e for e in events if e['event'] == 'navigation-placement-owner']
    archive = (ROOT / 'GAME_DATA/Bhunter.bin').read_bytes()
    index = json.loads((ROOT / 'analysis/assets-index.json').read_text())
    by_name = {x['name'].upper(): archive[x['offset']:x['end']] for x in index}
    names = []
    for line in by_name['BSP\\BLOCKS.TXT'].splitlines():
        if line.lower() == b'end':
            break
        if line and not line.startswith(b';'):
            names.append(line.decode())
    grids = []
    for name in names:
        text = by_name.get('BSP\\' + name[:-4].upper() + '.TXT', b'')
        grid = 10  # Verified registry initialization 00411f00 before script application.
        for line in text.splitlines():
            if line[:4].lower() == b'grid':
                match = re.fullmatch(rb'grid\s+([0-9]+)\s*', line, re.IGNORECASE)
                assert match, line
                grid = int(match[1])
        assert grid > 0
        grids.append(grid)
    lines = ['/* Existing startup observations and independent model-grid inputs. */',
             'static const long world_grids[454]={' + ','.join(map(str, grids)) + '};',
             '/* placement, model, ports, minimum/maximum float bits */',
             'static const unsigned long world_cases[1454][9]={']
    for node in nodes:
        bits = struct.unpack('<6I', struct.pack('<6f', *node['bounds']))
        lines.append('{' + ','.join('0x%xUL' % x for x in [node['placement'], node['model_index'], node['ports'], *bits]) + '},')
    lines += ['};', '/* sensor/launcher placement and resolved owner placement */', 'static const unsigned long world_owners[4][2]={']
    by_index = {n['index']: n for n in nodes}
    for owner in owners:
        lines.append('{%dUL,%dUL},' % (owner['placement'], by_index[owner['node_index']]['placement']))
    lines.append('};')
    data = ('\n'.join(lines) + '\n').encode()
    (ROOT / 'tests/world_geometry_cases.h').write_bytes(data)
    report = dict(nodes=len(nodes), owners=len(owners), models=len(grids),
                  source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                  header_sha256=hashlib.sha256(data).hexdigest(), archive_sha256=hashlib.sha256(archive).hexdigest(),
                  limits=['Grids are fixture inputs extracted from known grid statements; full block scripts remain unapplied.',
                          'Saved 9-significant-digit binary32 bounds roundtrip to binary32; original transforms were not captured.'],
                  game_launches=0)
    (ROOT / 'META/m4/construction/world-fixtures.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
