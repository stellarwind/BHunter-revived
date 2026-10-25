"""Independent PC53 numeric cases from pinned original instruction constants."""
import json,struct
from pathlib import Path
root=Path(__file__).resolve().parents[3];here=Path(__file__).resolve().parent
constants=json.loads((here/'constants.json').read_text())
def literal(address,kind='double'):return constants[address+'-'+kind]['value']
def f32(v):return struct.unpack('<f',struct.pack('<f',v))[0]
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
axis=[]
for raw in [0,1,2767,2768,27767,27768,27769,32767,32768,32769,37767,37768,37769,57767,57768,62768,65535,65536,0x7fffffff,0x80000000,0xffffffff]:
 for deadzone in [0,1]:
  x=float(raw)-literal('0048a000')
  if deadzone:
   if x>0:x=max(x-literal('0048a010'),0)
   else:x=min(x-literal('0048a018'),0)
  retained=x*literal('0048a020' if deadzone else '0048a038');stored=f32(retained)
  active=-1.0 if retained<literal('0048a028') else 1.0 if stored>literal('0048a030') else stored
  calx=float(raw)-literal('0048a000')
  calx=max(calx-literal('0048a010'),0) if calx>0 else min(calx-literal('0048a018'),0)
  calibration=max(-1.0,min(1.0,calx*literal('0048a020')))
  axis.append(dict(raw=raw,deadzone=deadzone,expected=bits(active),retained=retained,calibration=bits(calibration)))
mouse=[]
for old in [-1.010101079940796,-1.0101009607315063,-1.0,-.9999999403953552,-.01,0,.01,.9999999403953552,1.0,1.0101009607315063,1.010101079940796]:
 for delta in [-1,0,1,17,-17,0x7fffffff,-0x80000000]:
  old=f32(old)
  retained=(old-float(delta)*literal('0048a048'))*literal('0048a050');stored=f32(retained)
  final=-1.0 if retained< -1 else 1.0 if stored>1 else stored
  mouse.append(dict(previous=bits(old),delta=delta,expected=bits(final),retained=retained,stored=bits(stored),lower=retained< -1,upper=stored>1))
lines=['/* Generated independent PC53 literal/instruction oracle; no original execution. */',
 '#ifndef BH_INPUT_COLLECTION_CASES_H','#define BH_INPUT_COLLECTION_CASES_H',
 'typedef struct BHInputAxisCase {unsigned long raw,deadzone,expected,calibration;} BHInputAxisCase;',
 'static const BHInputAxisCase bh_input_axis_cases[]={']
for c in axis:lines.append('{0x%08xUL,%d,0x%08xUL,0x%08xUL},'%(c['raw'],c['deadzone'],c['expected'],c['calibration']))
lines+=['};','typedef struct BHInputMouseCase {unsigned long previous;long delta;unsigned long expected;} BHInputMouseCase;',
 'static const BHInputMouseCase bh_input_mouse_cases[]={']
for c in mouse:lines.append('{0x%08xUL,%s,0x%08xUL},'%(c['previous'],'(-2147483647L-1L)' if c['delta']==-0x80000000 else str(c['delta'])+'L',c['expected']))
lines+=['};','#endif','']
(root/'tests/input_collection_cases.h').write_text('\n'.join(lines))
(here/'numeric-case-evidence.json').write_text(json.dumps(dict(axis=axis,mouse=mouse,original_code_executed=False),indent=2)+'\n')
print('Generated',len(axis),'unsigned axis cases and',len(mouse),'mouse filter cases')
