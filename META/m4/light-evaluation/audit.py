"""Verify complete saved instruction exports against immutable PE bytes."""
import csv,hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];FOLDER=Path(__file__).resolve().parent
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
assert hashlib.sha256(exe).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',exe,60)[0];table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
sections=[struct.unpack_from('<IIII',exe,table+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
def original(address,size):
 rva=address-0x400000;_,start,length,offset=next(s for s in sections if s[1]<=rva and rva+size<=s[1]+s[2])
 return exe[offset+rva-start:offset+rva-start+size]
records=[]
for address,symbol in [(0x450cd0,'bh_light_evaluate_prepared'),(0x451140,'bh_light_evaluate_world')]:
 path=ROOT/('META/m4/light-prepare/original/%08x.tsv'%address);rows=list(csv.DictReader(path.open(),delimiter='\t'));at=address;body=bytearray()
 for row in rows:
  data=bytes.fromhex(row['bytes']);assert int(row['address'],16)==at and original(at,len(data))==data
  body.extend(data);at+=len(data)
 assert all(row['instruction']=='CALL 0x0047de20' for row in rows if row['instruction'].startswith('CALL'))
 records.append(dict(address='%08x'%address,symbol=symbol,role='engine_candidate',provenance_confidence='medium',classification='Complete ordinary lighting owner with callers in original renderer; proprietary/unknown engine provenance, no verified reusable library. Not import, CRT, EH/compiler helper or shared tail.',original=path.relative_to(ROOT).as_posix(),boundary='%08x-%08x'%(address,at-1),instructions=len(rows),bytes=len(body),body_sha256=hashlib.sha256(body).hexdigest()))
for address,expected in [(0x451128,(0x450d26,0x450d76,0x450dfc,0x450f0e)),(0x451594,(0x451193,0x4511e3,0x451269,0x45137b))]:
 assert struct.unpack('<4I',original(address,16))==expected
for address,fmt,expected in [(0x489ed0,'f',0),(0x489ee0,'d',1),(0x489ee8,'f',1)]:assert struct.unpack('<'+fmt,original(address,struct.calcsize(fmt)))[0]==expected
result=json.loads((FOLDER/'focused-build.json').read_text());assert result['passed'] and result['test']['checks']==2100 and not result['test']['failures']
for name,expected in result['sources'].items():assert sha(ROOT/name)==expected,name
review=dict(records=records,implementation='func',integration='focused_linked_harness; parent integration pending',target=result['target'],focused_checks=result['test'],dependencies=[dict(original='0047de20',role='library_verified',binding='VC5 CRT __ftol emitted by C integer conversion',note='Reused compiler/runtime support, excluded from recovered lighting count'),dict(original='x87 FSQRT',binding='sqrt VC5 intrinsic/runtime'),dict(original='004ede88',binding='BHLightContext.lights'),dict(original='004ede90 / 004e2df8',binding='BHLightContext.reference_override / local_reference'),dict(source='src/bh_round.c',binding='explicit binary32 store helper'),dict(source='src/bh_light.c, src/bh_math.c',binding='real prepare and transforms linked and exercised in prepare/evaluate chain')],source_sha256={name:sha(ROOT/name) for name in ('src/bh_light_eval.c','src/bh_light_eval.h','tests/light_eval.c')},original_code_executed=False,game_launches=0,limits=['PC53/nearest/masked finite consumed inputs; PC24/64 and FP sticky flags not claimed','Source guards reject scaled channels outside signed32 conversion range; original CRT low32 behavior for extreme signed64 conversion not admitted','No renderer/gameplay, real scene lighting integration, light allocation/list lifetime or byte match claimed','No original-code differential execution; independent analytical and instruction-derived FP contract controls only'])
(FOLDER/'review.json').write_text(json.dumps(review,indent=2)+'\n',newline='\n')
print(json.dumps(dict(passed=True,complete_functions=len(records),original_instructions=sum(r['instructions'] for r in records),records=records,focused_checks=result['test']['checks'])))
