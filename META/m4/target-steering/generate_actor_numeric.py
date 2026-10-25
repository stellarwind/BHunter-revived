"""Independent PC53 operation-graph fixtures for complete route promotion.

These are static-contract checks, not differential original execution.
"""
import json
import math
from pathlib import Path
import random
import struct
ROOT=Path(__file__).resolve().parents[3]
FOLDER=Path(__file__).resolve().parent
def f32(v):return struct.unpack('<f',struct.pack('<f',v))[0]
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def difference(a,b):return [f32(a[i]-b[i]) for i in range(3)]
def snap(v):
    length=math.sqrt((v[1]*v[1]+v[2]*v[2])+v[0]*v[0])
    magnitudes=list(map(abs,v));axis=0 if magnitudes[0]>magnitudes[1] and magnitudes[0]>magnitudes[2] else (1 if magnitudes[1]>magnitudes[2] else 2)
    result=[0,0,0];result[axis]=f32(length if v[axis]>0 else -length)
    return length,result
def tangent(p,d):return [f32(p[i]-d[i]*(-1.3)) for i in range(3)]
def main():
    rng=random.Random(0x434f40);cases=[]
    for index in range(24):
        axis=index%3;flags=8 if index%2 else 0;outgoing=0 if index%4==0 else 1
        old=[f32(rng.uniform(-100,100)) for i in range(3)]
        low=[f32(rng.uniform(-150,-50)) for i in range(3)];high=[f32(rng.uniform(50,150)) for i in range(3)]
        widths=[f32(rng.uniform(10,20) if index%3==0 else rng.uniform(40,70)) for i in range(2)]
        scratch=[f32(-widths[0]/2),f32(widths[0]/2),f32(-widths[1]/2),f32(widths[1]/2)]
        speed=f32(rng.uniform(5,50));scale=f32(rng.uniform(.25,3));u=f32(rng.random());v=f32(rng.random());plane=100 if outgoing else 0
        control_u=f32((1-u)*scratch[1]+u*scratch[0]);control_v=f32((1-v)*scratch[3]+v*scratch[2])
        center_u=f32((scratch[1]+scratch[0])*.5);center_v=f32((scratch[3]+scratch[2])*.5)
        other_axes=[j for j in range(3) if j!=axis]
        endpoint=[0,0,0];endpoint[axis]=plane;endpoint[other_axes[0]]=control_u;endpoint[other_axes[1]]=control_v
        portal_center=[0,0,0];portal_center[axis]=plane;portal_center[other_axes[0]]=center_u;portal_center[other_axes[1]]=center_v
        if flags and outgoing:
            endpoint=old[:];endpoint[axis]=plane;length,_=snap(difference(endpoint,old));length+=length
            first=endpoint[:];last=old[:]
        else:
            midpoint=[f32((low[0]+high[0])*.5),f32((endpoint[1]+old[1])*.5),f32((low[2]+high[2])*.5)]
            length_a,delta=snap(difference(midpoint,old));first=tangent(old,delta)
            length_b,delta=snap(difference(midpoint,endpoint));length=length_b+f32(length_a);last=tangent(endpoint,delta)
            if not outgoing:
                first=[f32(first[0]-delta[2]),f32(first[1]-2),f32(first[2]+delta[0])]
                last=[f32(last[0]+delta[2]),f32(last[1]+2),f32(last[2]-delta[0])]
        duration=f32(length/(speed*scale));ticks=int(duration*30)&65535
        # Both portal records share these scratch widths in the C fixture.
        width=f32(scratch[1]-scratch[0])
        if scratch[1]-scratch[0]<30:
            width=f32(max(width,scratch[3]-scratch[2]))
            if width<30:
                quotient=duration/((width+10)*.025);duration=f32(quotient);ticks=int(quotient*30)&65535
        case=dict(axis=axis,flags=flags,outgoing=outgoing,old=list(map(bits,old)),low=list(map(bits,low)),high=list(map(bits,high)),
            scratch=list(map(bits,scratch)),parameters=list(map(bits,[speed,scale,u,v])),
            controls=[list(map(bits,p)) for p in (old,first,last,endpoint)],center=list(map(bits,portal_center)),duration=bits(duration),ticks=ticks)
        cases.append(case)
    (FOLDER/'actor-numeric-fixtures.json').write_text(json.dumps(cases,indent=2)+'\n')
    def values(a):return '{'+','.join('0x%08x'%v for v in a)+'}'
    header='/* Independent PC53 promotion fixtures; generate_actor_numeric.py. */\n'
    for c in cases:
        fields=[str(c[k]) for k in ('axis','flags','outgoing')]+[values(c[k]) for k in ('old','low','high','scratch','parameters')]
        fields+=['{'+','.join(values(p) for p in c['controls'])+'}',values(c['center']),'0x%08x'%c['duration'],str(c['ticks'])]
        header+='    {'+','.join(fields)+'},\n'
    (ROOT/'tests/actor_route_cases.h').write_text(header)
if __name__=='__main__':main()
