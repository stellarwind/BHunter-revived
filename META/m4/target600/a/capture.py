import json,subprocess,hashlib,struct
from pathlib import Path
H=Path('META/m4/target600/a');p=Path('GAME_DATA/BHUNTER.EXE').read_bytes();pe=struct.unpack_from('<I',p,60)[0];s=pe+24+struct.unpack_from('<H',p,pe+20)[0];ss=[struct.unpack_from('<IIII',p,s+i*40+8) for i in range(struct.unpack_from('<H',p,pe+6)[0])]
def raw(a,n):
 for vs,va,sz,o in ss:
  if va<=a-0x400000<va+sz:return p[o+a-0x400000-va:o+a-0x400000-va+n]
rows=[]
for a,n in [(0x441d80,1454),(0x442360,223),(0x43e840,589),(0x440050,1973),(0x440ad0,360)]:
 d=json.loads(subprocess.check_output(['ghidra','--project','bhunter-frame','disasm',hex(a),'-n',str(n)],text=True));d=[r for r in d if a<=int(r['address'],16)<a+n];b=raw(a,n);assert b''.join(bytes.fromhex(r['bytes']) for r in d)==b
 row=dict(original_va=f'{a:08x}',role='game',bytes=n,sha256=hashlib.sha256(b).hexdigest(),instructions=len(d),end_exclusive=f'{a+n:08x}',classification='Whole ordinary projectile constructor or installed simulation/draw callback; constructor/call-site reviewed, no CRT/import/compiler/shared tail ownership.');rows.append(row)
 (H/'original'/f'{a:08x}.disasm.json').write_bytes((json.dumps(d,indent=2)+'\n').encode())
 (H/'original'/f'{a:08x}.decompile.json').write_bytes((json.dumps(json.loads(subprocess.check_output(['ghidra','--project','bhunter-frame','decompile',hex(a)],text=True)),indent=2)+'\n').encode())
(H/'classification.json').write_bytes((json.dumps(rows,indent=2)+'\n').encode())
constants=[]
for a in [0x48980c,0x489db0,0x489c58,0x489318,0x489208,0x489628,0x4892e8,0x489658,0x489988,0x489c80,0x4897f4,0x4893e8,0x489260,0x4892dc,0x489a90,0x489508,0x489280,0x489498,0x489350,0x489330]:
 b=raw(a,8);constants.append(dict(original_va=f'{a:08x}',raw=b.hex(),double=struct.unpack('<d',b)[0],float=struct.unpack('<f',b[:4])[0]))
(H/'constants.json').write_bytes((json.dumps(constants,indent=2)+'\n').encode());print(constants)
