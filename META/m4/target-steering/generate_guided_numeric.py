"""Independent PC53 guided-exit selection fixtures, no original execution."""
import json
from pathlib import Path
import random
import struct
ROOT=Path(__file__).resolve().parents[3]
FOLDER=Path(__file__).resolve().parent
def f32(v):return struct.unpack('<f',struct.pack('<f',v))[0]
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def choose(score):return 2 if score<f32(score) else 1
def main():
    rng=random.Random(0x434650);fixtures=[]
    for _ in range(10000):
        anchor=[f32(rng.uniform(-2e7,2e7)) for i in range(2)]
        point=[f32(rng.uniform(-2e7,2e7)) for i in range(2)]
        y,z=[point[i]-anchor[i] for i in range(2)]
        mixed=y*f32(y)+f32(z)*f32(z)
        symmetric=y*y+z*z;rounded=f32(y)*f32(y)+f32(z)*f32(z)
        selected=choose(mixed)
        if selected!=choose(symmetric) or selected!=choose(rounded):
            fixtures.append(dict(values=list(map(bits,anchor+point)),selected=selected,
                symmetric=choose(symmetric),rounded=choose(rounded)))
            if len(fixtures)==16:break
    assert len(fixtures)==16
    (FOLDER/'guided-numeric-fixtures.json').write_text(json.dumps(fixtures,indent=2)+'\n')
    header='/* Independent PC53 selection fixtures; generate_guided_numeric.py. */\n'
    for case in fixtures:header+='    {{'+','.join('0x%08x'%v for v in case['values'])+'},%d},\n'%case['selected']
    (ROOT/'tests/route_guided_cases.h').write_text(header)
if __name__=='__main__':main()
