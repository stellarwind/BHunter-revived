"""Reproduce independent spatial controls and verify entity family boundaries."""
from pathlib import Path
import csv
import hashlib
import json
import math
import random
import struct

ROOT=Path(__file__).resolve().parents[1]


def main():
    single=lambda v:struct.unpack('<f',struct.pack('<f',v))[0]
    bits=lambda v:struct.unpack('<I',struct.pack('<f',v))[0]
    value=lambda w:struct.unpack('<f',struct.pack('<I',w))[0]
    rng=random.Random(764770);rows=[]
    for _ in range(256):
        center=[single(rng.uniform(-80,80)) for _ in range(3)]
        position=[single(rng.uniform(-80,80)) for _ in range(3)]
        delta=[center[i]-position[i] for i in range(3)]
        distance=math.sqrt((delta[0]*single(delta[0])+delta[1]*delta[1])+delta[2]*single(delta[2]))
        nearest=bits(single(distance))
        for offset in (-1,0,1):
            radius=value(nearest+offset)
            hit=all(single(center[i]-radius)<=position[i]<=single(center[i]+radius) for i in range(3)) and distance<=radius
            rows.append('{ {'+','.join('0x%08xUL'%bits(v) for v in (*center,*position,radius))+'},'+str(int(hit))+'},')
    expected='/* Independent PC53 / observed store-width spatial-query controls. */\n'+'\n'.join(rows)+'\n'
    actual=(ROOT/'tests/entity_query_cases.h').read_bytes()
    assert actual==expected.encode()
    controls=json.loads((ROOT/'META/m2/entities/query-controls.json').read_text())
    assert hashlib.sha256(actual).hexdigest()==controls['header_sha256'] and controls['cases']==768
    review=json.loads((ROOT/'META/m2/entities/review.json').read_text())
    with (ROOT/'META/m2/entities/original/boundaries.tsv').open(encoding='utf-8-sig',newline='') as file:
        boundaries={v['address']:v for v in csv.DictReader(file,delimiter='\t')}
    for item in review['records']:
        assert item['body_ranges']==boundaries[item['address']]['ranges'].split('|')
        assert boundaries[item['address']]['thunk']=='false'
    assert next(v for v in review['records'] if v['address']=='00476d50')['implementation']=='excluded_from_family'
    print(json.dumps(dict(passed=True,source='independent Python spatial model and complete boundary catalog',query_cases=768,implemented_family_entries=7,excluded_directplay_entry='00476d50')))


if __name__=='__main__':main()
