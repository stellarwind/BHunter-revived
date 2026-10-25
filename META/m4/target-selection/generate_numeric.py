"""Independent binary64/PC53 fixtures from the reviewed x87 operation graph.

This does not execute original code and is not differential evidence. Select
fixtures which detect replacement with a uniformly rounded/full-precision norm.
"""
import json
import math
from pathlib import Path
import random
import struct

ROOT=Path(__file__).resolve().parents[3]
FOLDER=Path(__file__).resolve().parent
def f32(value):return struct.unpack('<f',struct.pack('<f',value))[0]
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def main():
    random_source=random.Random(0x421610)
    fixtures=[]
    for _ in range(100000):
        point=[f32(random_source.uniform(-2e7,2e7)) for _ in range(3)]
        origin=[f32(random_source.uniform(-2e7,2e7)) for _ in range(3)]
        x,y,z=[float(point[j])-origin[j] for j in range(3)]
        score=[bits(math.sqrt((x*f32(x)+y*y)+z*f32(z))),
               bits(math.sqrt((x*f32(x)+y*f32(y))+z*z))]
        rounded=bits(math.sqrt(sum(f32(t)*f32(t) for t in (x,y,z))))
        retained=bits(math.sqrt((x*x+y*y)+z*z))
        if score[0]!=score[1] or (score[0]!=rounded and score[1]!=retained):
            fixtures.append((list(map(bits,point)),list(map(bits,origin)),score,rounded,retained))
            if len(fixtures)==16:break
    assert len(fixtures)==16
    (FOLDER/'numeric-fixtures.json').write_text(json.dumps(fixtures,indent=2)+'\n')
    header='/* Generated binary64 PC53 arithmetic fixtures; see numeric-fixtures.json. */\n'
    for point,origin,score,_,_ in fixtures:
        fields=['{'+','.join('0x%08x'%v for v in field)+'}' for field in (point,origin,score)]
        header+='    {'+','.join(fields)+'},\n'
    (ROOT/'tests/target_selection_cases.h').write_text(header)
if __name__=='__main__':main()
