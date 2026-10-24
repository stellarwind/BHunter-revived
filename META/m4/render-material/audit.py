"""Check complete original body/data against preserved PE; source target evidence."""
import csv,hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];FOLDER=Path(__file__).resolve().parent
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert hashlib.sha256(exe).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',exe,60)[0];table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
sections=[struct.unpack_from('<IIII',exe,table+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
def original(address,size):
 rva=address-0x400000;_,start,length,offset=next(s for s in sections if s[1]<=rva and rva+size<=s[1]+s[2])
 return exe[offset+rva-start:offset+rva-start+size]
path=ROOT/'META/m4/render-state/original/0045b1d0.tsv';rows=list(csv.DictReader(path.open(),delimiter='\t'));at=0x45b1d0;body=bytearray()
for row in rows:
 data=bytes.fromhex(row['bytes']);assert int(row['address'],16)==at and original(at,len(data))==data;body.extend(data);at+=len(data)
assert len(rows)==195 and at==0x45b403
assert [r['instruction'] for r in rows if r['instruction'].startswith('CALL 0x')]==['CALL 0x0044e780']
for address,value in [(0x49181c,b' D3DERR: Create default material failed. \0'),(0x491848,b' VMEMERR: Alloc texture slots failed. \0')]:assert original(address,len(value))==value
result=json.loads((FOLDER/'focused-build.json').read_text());assert result['passed'] and result['test']['checks']==83 and not result['test']['failures']
for name,value in result['sources'].items():assert sha(ROOT/name)==value
record=dict(address='0045b1d0',symbol='bh_render_material_initialize',role='engine_candidate',provenance_confidence='medium',classification='Complete ordinary initialization owner called by00479200, native material/render-state behavior, unknown proprietary engine authorship. Not CRT/import/compiler support/shared tail.',original=path.relative_to(ROOT).as_posix(),boundary='0045b1d0-0045b402',instructions=len(rows),bytes=len(body),body_sha256=hashlib.sha256(body).hexdigest(),implementation='func',integration='focused_linked_harness; parent integration pending')
review=dict(records=[record],dependencies=[dict(original='0044e780',source='src/bh_texture.c',symbol='bh_texture_preallocate'),dict(original='004e32f4',binding='BHTextureContext.software'),dict(original='004e9c54 / 004e9c58',binding='borrowed real IDirect3D2 / BHTextureContext.device'),dict(original='004e9c70',binding='BHTextureContext.errors with bounded append guard'),dict(original='SDK vtables',binding='Real IDirect3D2 CreateMaterial, IDirect3DMaterial2 SetMaterial/GetHandle, IDirect3DDevice2 SetLightState/SetRenderState')],source_sha256=result['sources'],focused_checks=result['test'],target=result['target'],original_code_executed=False,game_launches=0,limits=['Original local material reference is leaked; source retains it in explicit owner released by separately labeled source adapter','CreateMaterial failure branch reconstructed statically; native COM allocation/device errors and HAL backend failure behavior not forced or claimed','All post-CreateMaterial HRESULTs ignored for original return and recorded separately by source diagnostics','Guarded arguments/reinitialization and bounded error append extend original valid-input contract','No source game or scene material integration claimed here; focused target links current real m0 owner objects and requires parent full rebuild'])
(FOLDER/'review.json').write_text(json.dumps(review,indent=2)+'\n',newline='\n')
print(json.dumps(dict(passed=True,record=record,focused_checks=result['test']['checks'])))
