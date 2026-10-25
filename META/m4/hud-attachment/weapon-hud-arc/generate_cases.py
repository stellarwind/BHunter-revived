"""Static PC53 curve dataflow probes from00432050, no executable invocation.
VC5 CRT RNG recurrence is independently positive-controlled by the C test.
"""
import random,struct,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[4];HERE=Path(__file__).resolve().parent
def f(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def word(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def curve(vectors,strength,seed,mode=None):
 def rnd():
  nonlocal seed;seed=(seed*214013+2531011)&0xffffffff;return (seed>>16)&32767
 strength=min(strength,1);points=[vectors[0][:]];rnd();t=0
 for i in range(7,-9,-1):
  #32123/3212d stored progress;32157 retained raw1-t and stored complement.
  t=f(t-(-strength/16));raw_s=1-t;s=f(raw_s);q=raw_s*s
  #32184 stores blend but32188 uses retained blend for1-blend.
  b=q*3-(q*s+q*s);blend=f(b);other=f(1-(blend if mode=='blend' else b))
  point=[]
  for axis in range(3):
   a=s*vectors[0][axis]+t*vectors[1][axis];d=s*vectors[2][axis]+t*vectors[3][axis]
   #321a7/321ef retain X line sums. Y/Z reload321c2/1dc/2208/2215/222b.
   rounded=(axis!=0)
   if mode=='x' and axis==0:rounded=True
   if mode=='yz' and axis!=0:rounded=False
   if rounded:a=f(a);d=f(d)
   point.append(f(a*blend+d*other))
  if i>-8:
   envelope=f(((64-i*i)*strength)*2**-21)
   point=[f((rnd()-16384)*envelope+x) for x in point]
  rnd();points.append(point)
 return points
rng=random.Random(0x432050);cases=[]
for mode in ['x','yz','blend']:
 for _ in range(10000):
  vectors=[[f(rng.uniform(-40000,40000)) for _ in range(3)] for _ in range(4)]
  strength=f(rng.uniform(.01,1));seed=rng.randrange(1,65536)
  actual=curve(vectors,strength,seed);naive=curve(vectors,strength,seed,mode)
  differs=[(i,axis) for i in range(17) for axis in range(3) if word(actual[i][axis])!=word(naive[i][axis])]
  if differs:
   probes=[dict(point=i,axis=axis,expected=word(actual[i][axis]),incorrect=word(naive[i][axis])) for i,axis in differs[:4]]
   cases.append(dict(name='retained_'+mode,vectors=vectors,strength=strength,seed=seed,probes=probes));break
 else:raise AssertionError(mode)
(HERE/'static-cases.json').write_text(json.dumps(dict(oracle='static x87 dataflow with explicit FST and pinned VC5 CRT RNG recurrence; no original execution',cases=cases),indent=2)+'\n')
def words(v):return '{'+','.join('0x%08xUL'%word(x) for x in v)+'}'
lines=['/* Static instruction-derived PC53 curve fixtures, not original-runtime output. */',
 'typedef struct ArcProbe {unsigned long point,axis,expected;} ArcProbe;',
 'typedef struct ArcCase {const char *name;unsigned long vectors[4][3],strength,seed,count;ArcProbe probes[4];} ArcCase;',
 'static const ArcCase arc_cases[]={']
for c in cases:
 probes=['{%d,%d,0x%08xUL}'%(p['point'],p['axis'],p['expected']) for p in c['probes']]
 lines.append('{"%s",{%s},0x%08xUL,%d,%d,{%s}},'%(c['name'],','.join(words(v) for v in c['vectors']),word(c['strength']),c['seed'],len(probes),','.join(probes)))
lines.append('};');(ROOT/'tests/weapon_hud_arc_cases.h').write_text('\n'.join(lines)+'\n')
print(json.dumps(dict(cases=len(cases),probes=sum(len(c['probes']) for c in cases),original_code_executed=False)))
