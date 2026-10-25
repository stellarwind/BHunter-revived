"""Audit immutable instruction bodies, UV contract and fresh source variants."""
import csv,hashlib,json,struct
from pathlib import Path
root=Path(__file__).resolve().parents[3];here=Path(__file__).resolve().parent
exe=(root/'GAME_DATA/BHUNTER.EXE').read_bytes()
assert hashlib.sha256(exe).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
p=struct.unpack_from('<I',exe,60)[0];t=p+24+struct.unpack_from('<H',exe,p+20)[0]
sections=[struct.unpack_from('<IIII',exe,t+40*i+8) for i in range(struct.unpack_from('<H',exe,p+6)[0])]
def raw(va,n):
 rva=va-0x400000;_,base,_,offset=next(s for s in sections if s[1]<=rva and rva+n<=s[1]+s[2]);return exe[offset+rva-base:offset+rva-base+n]
def owner(address,rows):
 at=int(address,16);body=bytearray()
 for row in rows:
  assert int(row['address'],16)==at
  b=bytes.fromhex(row['bytes']);body.extend(b);at+=len(b)
 assert body==raw(int(address,16),len(body))
 return dict(address=address,end_inclusive=f'{at-1:08x}',bytes=len(body),instructions=len(rows),body_sha256=hashlib.sha256(body).hexdigest())
owners=[]
for a in ['0041b650','0041b2d0']:
 rows=json.loads((here/'original'/(a+'.disasm.json')).read_text());assert rows[-1]['mnemonic']=='RET';owners.append(owner(a,rows))
assert owners[0]['bytes']==1434 and owners[1]['bytes']==893
for a in ['00454fc0','00478f60','004776c0']:
 rows=list(csv.DictReader((here/'original'/(a+'.tsv')).open(),delimiter='\t'));assert rows[-1]['instruction']=='RET';owners.append(owner(a,rows))
constants=json.loads((here/'constants.json').read_text())
for a,value in constants.items():assert raw(int(a,16),value['size']).hex()==value['bytes']
results={}
for name in ['rect-uv-validation','radar-validation']:
 data=json.loads((here/(name+'.json')).read_text());assert data['passed']
 for path,digest in data['source_sha256'].items():assert hashlib.sha256((root/path).read_bytes()).hexdigest()==digest,(name,path)
 results[name]=dict(passed=True,outputs=[c['stdout'] for c in data['commands'] if c['args'][0].lower().endswith('.exe') and c['args'][0].lower().startswith(str(root/'build').lower())])
sources=['src/bh_radar.c','src/bh_radar.h','src/bh_pickup_draw.c','src/bh_pickup_draw.h','tests/radar.c','tests/radar_native.c','tests/rect_uv_native.c','tests/pickup_draw.c']
evidence=dict(schema_version=1,original_instruction_owners=owners,admitted_candidate=dict(address='0041b650',role='game',provenance='high: direct player callback and game HUD/radar traversal callers, actual radar image and HUD placement state',implementation='func',source='src/bh_radar.c',symbol='bh_radar_marker',integration='fresh focused source-only linked harness; parent integration admission separately required'),setup_blocker=dict(address='0041b2d0',implementation='not_started',reason='full owner includes unrecovered00417f70 attachment drawing and00417f30 attachment lifecycle; explicit frame binding adapter does not implement this owner'),rect_correction=dict(source='src/bh_pickup_draw.c',changed_symbol='bh_pickup_rect_draw',implementation='adapter',original_address='00454fc0',behavior='U-start/U-extent/V-start/V-extent positional words were misinterpreted as U-start/V-start/U-extent/V-extent; corrected original sum and flip/transpose interpretation',unchanged_raw_contracts=['00478f60 producer payload words','004776c0 consumer argument forwarding','00440c40 pickup label literal/variable positional payloads','004412f0 pickup owner submissions'],prior_snapshot_sha256={str(p.relative_to(here)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted((here/'prior').glob('*'))},validation_policy='new linked source variant only; no historical byte/functional evidence transferred'),source_sha256={s:hashlib.sha256((root/s).read_bytes()).hexdigest() for s in sources},focused_results=results,original_code_executed=False,game_launches=0,limits=['software dispatch/raster backend still unfinished','HUD setup and attachment/weapon callbacks still unfinished','no full player scene callback or game target admission','PC24 and exceptional original pointer/nonfinite behavior not established'])
(here/'evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
print(json.dumps(dict(new_complete_owner='0041b650',verified_instruction_bodies=len(owners),focused_targets=5,original_code_executed=False,game_launches=0)))
