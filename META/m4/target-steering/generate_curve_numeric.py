"""Independent PC53 source-contract curve/chord fixtures, no original execution."""
import json
import math
from pathlib import Path
import random
import struct
ROOT=Path(__file__).resolve().parents[3]
FOLDER=Path(__file__).resolve().parent
def f32(v):return struct.unpack('<f',struct.pack('<f',v))[0]
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def curve(p,t):
    a,b,c,d=p
    if t==0:return a[:]
    om=f32(1-t);square=f32(t*t);blend=f32(square*3-(t*square+t*square));inverse=f32(1-blend)
    first=[f32(a[i]*om+b[i]*t) for i in range(3)]
    last=[d[0]*t+c[0]*om,c[1]*om+d[1]*t,f32(c[2]*om+d[2]*t)]
    return [f32((1-blend)*first[0]+last[0]*blend),f32(inverse*first[1]+last[1]*blend),f32(inverse*first[2]+last[2]*blend)]
def length(p,mixed):
    previous=p[0];total=0
    for t in (.25,.5,.75):
        current=curve(p,t);x,y,z=[current[i]-previous[i] for i in range(3)]
        squared=(x*f32(x)+y*f32(y))+z*z if mixed else (x*x+y*y)+z*z
        total=f32(math.sqrt(squared)+total);previous=current
    return bits(total)
def main():
    rng=random.Random(0x434810);fixtures=[]
    for _ in range(10000):
        controls=[[f32(rng.uniform(-2e7,2e7)) for i in range(3)] for j in range(4)]
        actual=length(controls,True);symmetric=length(controls,False)
        if actual!=symmetric:
            fixtures.append(dict(controls=[[bits(v) for v in p] for p in controls],score=actual,symmetric=symmetric))
            if len(fixtures)==16:break
    assert len(fixtures)==16
    (FOLDER/'curve-numeric-fixtures.json').write_text(json.dumps(fixtures,indent=2)+'\n')
    header='/* Independent PC53 operation-graph fixtures; generate_curve_numeric.py. */\n'
    for case in fixtures:
        controls=','.join('{'+','.join('0x%08x'%v for v in p)+'}' for p in case['controls'])
        header+='    {{'+controls+'},0x%08x},\n'%case['score']
    (ROOT/'tests/route_curve_length_cases.h').write_text(header)
if __name__=='__main__':main()
