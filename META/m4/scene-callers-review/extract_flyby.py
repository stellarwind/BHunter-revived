import hashlib,json,re,struct
from pathlib import Path
root=Path(__file__).resolve().parents[3];here=root/'META/m4/flyby-camera';here.mkdir(exist_ok=True)
exe=(root/'GAME_DATA/BHUNTER.EXE').read_bytes();pe=struct.unpack_from('<I',exe,60)[0];o=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
sections=[struct.unpack_from('<IIII',exe,o+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
def raw(a,n):
 rva=a-0x400000;_,r,sz,f=next(q for q in sections if q[1]<=rva and rva+n<=q[1]+q[2]);return exe[f+rva-r:f+rva-r+n]
rows=json.loads((root/'META/m4/scene-callers-review/original/00429fb0.disasm.json').read_text(encoding='utf-8-sig'))
cs={};body=bytearray();at=0x429fb0
for row in rows:
 assert int(row['address'],16)==at;b=bytes.fromhex(row['bytes']);body.extend(b);at+=len(b)
 for op in row['operands']:
  m=re.search(r'(float|double) ptr \[0x(00489[a-f0-9]+)\]',op)
  if m:
   kind,a=m.groups();b=raw(int(a,16),4 if kind=='float' else 8)
   cs[a]=dict(kind=kind,bytes=b.hex(),value=struct.unpack('<f' if kind=='float' else '<d',b)[0])
assert len(body)==2556 and len(rows)==757 and raw(0x429fb0,len(body))==body
ptrs=struct.unpack('<15I',raw(0x48f9f8,60));strings=[]
for a in ptrs:
 b=raw(a,4096).split(b'\0')[0];strings.append(dict(address=f'{a:08x}',bytes=b.hex(),text=b.decode('ascii')))
(here/'original-review.json').write_text(json.dumps(dict(address='00429fb0',end=f'{at-1:08x}',bytes=len(body),instructions=len(rows),body_sha256=hashlib.sha256(body).hexdigest(),constants=cs,credits=strings),indent=2)+'\n')
print(json.dumps(dict(constants=cs,credits=strings),indent=2))
header='/* Literal credits read from original pointer table0048f9f8; no localization or authored replacement. */\nstatic const char *const flyby_credits[15]={\n'
header+=''.join('    '+json.dumps(s['text'])+',\n' for s in strings)+'};\n'
(root/'src/bh_flyby_credits.h').write_text(header)
