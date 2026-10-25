"""Verify original owner bytes/constants and record current source evidence."""
import hashlib,json,struct
from pathlib import Path
folder=Path(__file__).resolve().parent;root=folder.parents[2]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
exe=(root/'GAME_DATA/BHUNTER.EXE').read_bytes();pe=struct.unpack_from('<I',exe,60)[0]
table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
sections=[struct.unpack_from('<IIII',exe,table+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
def raw(va,size):
 rva=va-0x400000;_,start,_,offset=next(s for s in sections if s[1]<=rva and rva+size<=s[1]+s[2])
 return exe[offset+rva-start:offset+rva-start+size]
rows=json.loads((folder/'original/004296a0.disasm.json').read_text(encoding='utf-8-sig'))
body=bytearray();at=0x4296a0
for row in rows:
 assert int(row['address'],16)==at;b=bytes.fromhex(row['bytes']);body.extend(b);at+=len(b)
assert len(body)==2307 and len(rows)==701 and at==0x429fa3 and rows[-1]['mnemonic']=='RET'
assert raw(0x4296a0,len(body))==body
constants={}
for va,kind,want in [(0x489ae0,'d',4.666666666666667),(0x489350,'f',-1),(0x489628,'f',-2),
 (0x489308,'f',10),(0x4894f0,'f',-10),(0x489340,'d',-.5),(0x489280,'d',.5)]:
 b=raw(va,8 if kind=='d' else 4);value=struct.unpack('<'+kind,b)[0];assert value==want
 constants[f'{va:08x}']=dict(type=kind,bytes=b.hex(),value=value)
calls=sorted({row['operands'][0] for row in rows if row['mnemonic']=='CALL'})
assert calls==['0x0040a8c0','0x00433f70','0x00434040','0x0047de20']
sources=['src/bh_camera_route.c','src/bh_camera_route.h','tests/camera_route.c','tests/camera_route_real.c','src/bh_camera_curve.c','src/bh_camera_curve.h']
review=dict(address='004296a0',end='00429fa2',bytes=len(body),instructions=len(rows),body_sha256=hashlib.sha256(body).hexdigest(),
 role='engine_candidate',provenance_confidence='Complete ordinary path/camera geometry owner; engine authorship unknown; import/CRT/compiler/shared-tail excluded.',
 calls=calls,constants=constants,source_sha256={p:sha(root/p) for p in sources},
 source_target='build/camera-route/camera_route.exe',real_target='build/camera-route/camera_route_real.exe',
 focused_checks=15,real_checks=31,original_code_executed=False,game_launches=0,
 integration_admission='Parent current integration build and ledger required',raw_byte_match=False)
(folder/'review.json').write_text(json.dumps(review,indent=2)+'\n')
print(json.dumps(dict(passed=True,address='004296a0',bytes=len(body),instructions=len(rows),calls=calls,focused_checks=15,real_checks=31)))
