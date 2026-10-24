"""Read-only, independent struct-based inspection of pinned LEV/PTH data."""
from pathlib import Path
import collections
import csv
import hashlib
import json
import math
import struct
import zlib

ROOT=Path(__file__).resolve().parents[1]


def main():
    manifest=json.loads((ROOT/'META/reference-manifest.json').read_text())
    review=json.loads((ROOT/'META/m2/scene/review.json').read_text())
    archive=(ROOT/'GAME_DATA/Bhunter.bin').read_bytes()
    assert hashlib.sha256(archive).hexdigest()==next(v['sha256'] for v in manifest['files'] if v['path']=='GAME_DATA/Bhunter.bin')
    with (ROOT/'META/m2/scene/original/boundaries.tsv').open(encoding='utf-8-sig',newline='') as file:
        boundaries={v['address']:v for v in csv.DictReader(file,delimiter='\t')}
    expected={'00407d90':'00407d90-00407f25','00407f30':'00407f30-004082c5','004100d0':'004100d0-0041058f'}
    for address,ranges in expected.items():assert boundaries[address]['ranges']==ranges and boundaries[address]['thunk']=='false'
    captured={}
    for item in review['real_assets']:
        data=archive[item['offset']:item['end']]
        assert len(data)==item['size']
        assert hashlib.sha256(data).hexdigest()==item['sha256'] and '%08x'%(zlib.crc32(data)&0xffffffff)==item['crc32']
        captured[item['name'].upper()]=data
    data=captured['TEST.LEV'];count=struct.unpack_from('<i',data)[0]
    placements=list(struct.iter_unpack('<16siiiI',data[4:4+count*32]))
    links=list(struct.iter_unpack('<I8h',data[4+count*32:]))
    assert count==5815 and len(links)==543 and all(v[0]<count for v in links)
    assert all(b'\0' in v[0] for v in placements)
    data=captured['TEST.PTH'];nodes=struct.unpack_from('<i',data)[0]
    cells=list(struct.iter_unpack('<if',data[4:]))
    assert nodes==283 and len(cells)==nodes*nodes
    assert all(-1<=slot<=5 and math.isfinite(distance) and distance>=0 for slot,distance in cells)
    nonzero_diagonal=[dict(index=i,exit_slot=cells[i*nodes+i][0],distance=cells[i*nodes+i][1]) for i in range(nodes) if cells[i*nodes+i][1]!=0]
    unequal_pairs=sum(cells[i*nodes+j][1]!=cells[j*nodes+i][1] for i in range(nodes) for j in range(i))
    print(json.dumps(dict(passed=True,scope='data inspection only; no original-code execution or entity/graph construction',reviewed_complete_boundaries=expected,
        placements=count,link_records=len(links),link_value_range=[min(v for row in links for v in row[1:]),max(v for row in links for v in row[1:])],
        route_nodes=nodes,route_cells=len(cells),exit_slots=dict(collections.Counter(v[0] for v in cells)),
        nonzero_diagonal=nonzero_diagonal,asymmetric_distance_pairs=unequal_pairs,
        assets=review['real_assets']),indent=2))


if __name__=='__main__':main()
