"""Static scalar instruction-operation timer oracle; no original execution."""
from pathlib import Path
import hashlib,json,struct
ROOT=Path(__file__).resolve().parents[4];HERE=Path(__file__).resolve().parent
x=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
assert hashlib.sha256(x).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
p=struct.unpack_from('<I',x,60)[0];t=p+24+struct.unpack_from('<H',x,p+20)[0]
ss=[struct.unpack_from('<IIII',x,t+i*40+8) for i in range(struct.unpack_from('<H',x,p+6)[0])]
def raw(a,n):
 _,s,_,o=next(s for s in ss if s[1]<=a-0x400000 and a-0x400000+n<=s[1]+s[2])
 return x[o+a-0x400000-s:o+a-0x400000-s+n]
step=struct.unpack('<d',raw(0x4891f8,8))[0]
assert step==0.033333333333333333 and raw(0x489200,4)==bytes(4)
words=[0,0x80000000,0x3f800000,0x3d088888,0x3d088889,0x3d08888a,1,0x7fffff,
       0x800000,0x3c23d70a,0x3f000000,0x40000000,0x7f7fffff,0x7f800000,
       0xff800000,0xbf800000,0x7fc12345,0xffc54321,0xff7fffff,0x4b000000]
def tick(word):
 value=struct.unpack('<f',struct.pack('<I',word))[0]
 return struct.unpack('<I',struct.pack('<f',value-step))[0] if value>0 else word
cases=[(w,tick(w)) for w in words];sequence=[];w=0x3f800000
for i in range(40):w=tick(w);sequence.append(w)
header='/* Static original-operation data oracle; not original code execution. */\n'
header+='static const unsigned long latch_cases[][2]={\n'+''.join(' {0x%08xUL,0x%08xUL},\n'%r for r in cases)+'};\n'
header+='static const unsigned long latch_sequence[]={\n'+''.join(' 0x%08xUL,\n'%w for w in sequence)+'};\n'
(ROOT/'tests/weapon_latch_cases.h').write_text(header)
(HERE/'static-cases.json').write_text(json.dumps(dict(step_address='004891f8',step_raw=raw(0x4891f8,8).hex(),step_value=step,
 cases=[dict(input=f'{a:08x}',output=f'{b:08x}') for a,b in cases],sequence=[f'{w:08x}' for w in sequence],
 original_code_executed=False,limits='Scalar PC53 operation oracle; not universal x87/PC24 proof.'),indent=2)+'\n')
print('Generated20 raw-word timer cases and40 sequential steps')
