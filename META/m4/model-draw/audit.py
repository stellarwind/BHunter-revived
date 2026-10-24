"""Verify complete model draw owners and isolated source contract target."""
import csv,hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];FOLDER=Path(__file__).resolve().parent
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();pe=struct.unpack_from('<I',exe,60)[0]
table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
sections=[struct.unpack_from('<IIII',exe,table+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
def original(address,size):
    rva=address-0x400000;_,start,length,offset=next(s for s in sections if s[1]<=rva and rva+size<=s[1]+s[2])
    return exe[offset+rva-start:offset+rva-start+size]
records=[];bodies={}
for address,end,count,api in [('00458310','00458421',96,'bh_model_draw_face'),('00458430','00458460',20,'bh_model_draw_leaf_chain'),('00458470','004593fe',1184,'bh_model_draw_unlit')]:
    at=int(address,16);body=bytearray();rows=list(csv.DictReader((FOLDER/'original'/f'{address}.tsv').open(),delimiter='\t'))
    for row in rows:
        data=bytes.fromhex(row['bytes']);assert int(row['address'],16)==at and original(at,len(data))==data
        body.extend(data);at+=len(data)
    assert at-1==int(end,16) and len(rows)==count;bodies[address]=rows
    records.append(dict(address=address,boundary=f'{address}-{end}',bytes=len(body),instructions=len(rows),body_sha256=hashlib.sha256(body).hexdigest(),role='engine_candidate',provenance_confidence='medium: ordinary application rendering-model owner with verified model-relative offsets, matrix/clipper/render calls and recursive BSP control flow; library attribution unresolved',boundary_review='complete contiguous instructions through RET, independent ordinary owner, no CRT/EH/import/shared-tail assignment',api=api,source='src/bh_model_draw.c',implementation='wip',integration='linked_harness',target='bhunter-model-draw.exe',validation='focused-build.json',game_admitted=False,pending='Parent-owned composed real native emitter target and current integration build; focused callback capture is isolated evidence, not production dependency admission'))
wrapper=list(csv.DictReader((FOLDER/'original/0045aa60.tsv').open(),delimiter='\t'))
for row in wrapper:
    data=bytes.fromhex(row['bytes']);assert original(int(row['address'],16),len(data))==data
assert any(row['instruction']=='LEA EBX,[EBP + 0x8]' for row in wrapper)
assert any(row['instruction']=='MOV dword ptr [0x004edeb0],EBX' for row in wrapper)
assert any(row['address']=='0045ad09' and row['instruction']=='INC ECX' for row in wrapper)
assert any(row['instruction']=='CMP ECX,EAX' and row['address']=='0045839a' for row in bodies['00458310'])
assert [r['instruction'] for r in bodies['00458470'] if r['instruction'].startswith('AND EBX,')]==['AND EBX,0xfffe','AND EBX,0xfffd','AND EBX,0xfffb','AND EBX,0xfff7','AND EBX,0xffef','AND EBX,0xffdf']
build=json.loads((FOLDER/'focused-build.json').read_text());assert build['passed'] and build['scenario']['checks']==171666 and not build['scenario']['failures']
for source,digest in build['sources'].items():assert sha(ROOT/source)==digest,source
assert build['commands'][-1]['exit_code'] and 'LNK2001' in build['commands'][-1]['output']
for entry in json.loads((ROOT/'META/reference-manifest.json').read_text())['files']:
    if entry['root']=='repo':assert sha(ROOT/entry['path'])==entry['sha256']
review=dict(records=records,original_exe_sha256=sha(ROOT/'GAME_DATA/BHUNTER.EXE'),sources=build['sources'],complete_source_candidates=3,focused_checks=171666,game_launches=0,original_code_executed=False,adapter_apis=['bh_model_draw_cache_init/dispose/reset','bh_model_draw_context_init','BHModelDrawCache typed runtime records and explicit traversal stack','BHModelPolygonEmit synchronous native dispatch binding with failure propagation'],limits=['Focused source target uses explicitly isolated capture callback; parent real native composition/full integration required before original-owner admission','Whole0046e390 software dispatch/raster kernels remain unfinished; no backend success stub is supplied','PC53/nearest/masked only; no byte equivalence or original differential execution','Source cache invalidates initial epochs and never interprets serialized pointer words; model arrays must remain stable and validated acyclic while cache lives','Original32bit frame integerbits retained via memcpy in float-declared vertex.frame; callers use nonzero epochs and reset before token reuse','No independent face backface removal: original BSP eye-side chooses visible leaf chain and preserves painter order'])
(FOLDER/'review.json').write_text(json.dumps(review,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(passed=True,complete_source_candidates=3,instructions_verified=sum(r['instructions'] for r in records),focused_checks=171666,production_binding_admitted=False,original_code_executed=False,immutable_references_verified=True)))
