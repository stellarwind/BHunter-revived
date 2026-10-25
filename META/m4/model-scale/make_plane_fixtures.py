"""Independent binary32 spill + sequential binary64 PC53 model; no original execution."""
import json,math,random,struct
from pathlib import Path
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
rng=random.Random(470000)
rows=[]
for k in range(12):
 raw=[rng.uniform(-1,1) for _ in range(3)];length=math.sqrt(sum(v*v for v in raw));normal=[f32(v/length) for v in raw]
 plane=f32(rng.uniform(-10000,10000));scale=[f32(rng.uniform(.01,8)*(-1 if (k>>i)&1 else 1)) for i in range(3)]
 point=[f32(n*-plane) for n in normal]
 divided=[f32(n/s) for n,s in zip(normal,scale)]
 length=math.sqrt((divided[0]*divided[0]+divided[1]*divided[1])+divided[2]*divided[2]);reciprocal=1.0/length
 normalized=[f32(reciprocal*n) for n in divided]
 term=[(p*s)*n for p,s,n in zip(point,scale,normalized)]
 result=f32(-((term[2]+term[0])+term[1]))
 rows.append(dict(normal=[bits(v) for v in normal],plane=bits(plane),scale=[bits(v) for v in scale],normalized=[bits(v) for v in normalized],result=bits(result)))
(Path(__file__).parent/'plane-fixtures.json').write_text(json.dumps(dict(profile='nearest binary32 stores, sequential IEEE binary64 operations; PC53',rows=rows),indent=2)+'\n',newline='\n')
print('\n'.join(' {'+','.join('0x%08xUL'%v for v in row['normal']+[row['plane']]+row['scale']+row['normalized']+[row['result']])+'},' for row in rows))
