import struct,math,random,json
from pathlib import Path
f=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
b=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
r=random.Random(881)
cases=[]
for threshold,charge in [(.02,0),(.075,48)]:
 for n in range(250000):
  p=[f(r.uniform(-1,1)),f(r.uniform(-1,1)),f(r.uniform(-1,1))];t=[f(r.uniform(40,140)),0,p[2]]
  target_y=p[1]+(t[0]-p[0])*math.sqrt(1/(1-threshold)**2-1)
  t[1]=f(target_y);dx=t[0]-p[0];dy=t[1]-p[1];dz=0.;sx=f(dx);sy=f(dy)
  rawlen=math.sqrt((dx*sx+dy*sy)+dz*dz);dot=sx
  angle=f(1-dot/rawlen);naive=f(1-sx/math.sqrt(sx*sx+sy*sy))
  keep=angle<=threshold;old=naive<=threshold
  if keep!=old:
   cases.append(dict(threshold=threshold,charge=charge,player=[b(x) for x in p],target=[b(x) for x in t],angle=b(angle),naive_angle=b(naive),retains=keep));break
p=Path('META/m4/hud-attachment/weapon-target-lifecycle');p.joinpath('precision-cases.json').write_text(json.dumps(dict(method='Independent PC53 scalar mixed raw/stored delta trace; not original execution',cases=cases),indent=2)+'\n')
print(json.dumps(cases))

h="typedef struct TargetPrecisionCase {unsigned long player[3],target[3];long charge;} TargetPrecisionCase;\nstatic const TargetPrecisionCase target_precision_cases[]={\n"
for c in cases:h+="{{%s},{%s},%d},\n"%(','.join('0x%08x'%v for v in c['player']),','.join('0x%08x'%v for v in c['target']),c['charge'])
h+='};\n';Path('tests/weapon_target_lifecycle_cases.h').write_text(h)
