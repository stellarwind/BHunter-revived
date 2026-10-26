import json,subprocess,hashlib,struct
from pathlib import Path
H=Path('META/m4/target600/a10');p=Path('GAME_DATA/BHUNTER.EXE').read_bytes();pe=struct.unpack_from('<I',p,60)[0];s=pe+24+struct.unpack_from('<H',p,pe+20)[0];ss=[struct.unpack_from('<IIII',p,s+i*40+8) for i in range(struct.unpack_from('<H',p,pe+6)[0])]
def raw(a,n):
 for vs,va,sz,o in ss:
  if va<=a-0x400000<va+sz:return p[o+a-0x400000-va:o+a-0x400000-va+n]
rows=[]
for a,n in [(4397072, 808), (4403648, 1626), (4405328, 678)]:
 d=json.loads(subprocess.check_output(['ghidra','--project','bhunter-frame','disasm',hex(a),'-n',str(n)],text=True));d=[r for r in d if a<=int(r['address'],16)<a+n];b=raw(a,n);assert b''.join(bytes.fromhex(r['bytes']) for r in d)==b
 row=dict(original_va=f'{a:08x}',role='game',bytes=n,sha256=hashlib.sha256(b).hexdigest(),instructions=len(d),end_exclusive=f'{a+n:08x}',classification='Whole ordinary projectile constructor or installed simulation/draw callback; constructor/call-site reviewed, no CRT/import/compiler/shared tail ownership.');rows.append(row)
 (H/'original'/f'{a:08x}.disasm.json').write_bytes((json.dumps(d,indent=2)+'\n').encode())
 (H/'original'/f'{a:08x}.decompile.json').write_bytes((json.dumps(json.loads(subprocess.check_output(['ghidra','--project','bhunter-frame','decompile',hex(a)],text=True)),indent=2)+'\n').encode())
(H/'classification.json').write_bytes((json.dumps(rows,indent=2)+'\n').encode())
constants=[]
for a in [4757904, 4758176, 4756232, 4756392, 4756720]:
 b=raw(a,8);constants.append(dict(original_va=f'{a:08x}',raw=b.hex(),double=struct.unpack('<d',b)[0],float=struct.unpack('<f',b[:4])[0]))
(H/'constants.json').write_bytes((json.dumps(constants,indent=2)+'\n').encode());print(constants)
