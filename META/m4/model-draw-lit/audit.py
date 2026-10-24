"""Verify complete lit traversal and focused real-dependency target."""
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
at=0x459510;body=bytearray();rows=list(csv.DictReader((FOLDER/'original/00459510.tsv').open(),delimiter='\t'))
for row in rows:
    data=bytes.fromhex(row['bytes']);assert int(row['address'],16)==at and original(at,len(data))==data
    body.extend(data);at+=len(data)
assert at==0x45a7b4 and len(rows)==1401 and len(body)==4772
calls=[r['instruction'] for r in rows if r['instruction'].startswith('CALL ')]
assert calls.count('CALL 0x00459510')==2 and calls.count('CALL 0x00450cd0')==2 and calls.count('CALL 0x0047de20')==6
assert calls.count('CALL 0x00458310')==2 and calls.count('CALL 0x00458430')==2
assert [r['instruction'] for r in rows if r['instruction'].startswith('AND ECX,')]==['AND ECX,0xfffe','AND ECX,0xfffd','AND ECX,0xfffb','AND ECX,0xfff7','AND ECX,0xffef','AND ECX,0xffdf']
for address in ('0045a451','0045a620'):
    assert next(r['instruction'] for r in rows if r['address']==address)=='LEA EAX,[ESI + 0x18]'
assert next(r['instruction'] for r in rows if r['address']=='0045a6b6')=='FCHS'
assert next(r['instruction'] for r in rows if r['address']=='0045a596')=='MOV dword ptr [0x004e3898],EDI'
assert next(r['instruction'] for r in rows if r['address']=='0045a767')=='MOV dword ptr [0x004e3898],EDI'
assert struct.unpack('<f',original(0x489ef8,4))[0]==.5
build=json.loads((FOLDER/'focused-build.json').read_text());assert build['passed'] and build['scenario']['checks']==43338 and not build['scenario']['failures']
for source,digest in build['sources'].items():assert sha(ROOT/source)==digest,source
assert build['commands'][-1]['exit_code'] and 'LNK2001' in build['commands'][-1]['output']
for entry in json.loads((ROOT/'META/reference-manifest.json').read_text())['files']:
    if entry['root']=='repo':assert sha(ROOT/entry['path'])==entry['sha256']
record=dict(address='00459510',boundary='00459510-0045a7b3',bytes=len(body),instructions=len(rows),body_sha256=hashlib.sha256(body).hexdigest(),role='engine_candidate',provenance_confidence='medium: complete application lit BSP traversal with real prepared-light evaluator and face/list owners; library attribution unresolved; CRTftol reused not counted',boundary_review='complete contiguous1401instructions, ordinary independent recursive owner; no CRT/EH/import/shared-tail assignment',api='bh_model_draw_lit',source='src/bh_model_draw_lit.c',implementation='wip',integration='linked_harness',target='bhunter-model-draw-lit.exe',validation='focused-build.json',game_admitted=False,pending='Parent composed real native emitter/current integration and actual scene controls; focused capture is isolated and not production binding admission')
review=dict(records=[record],original_exe_sha256=sha(ROOT/'GAME_DATA/BHUNTER.EXE'),sources=build['sources'],complete_source_candidates=1,focused_checks=43338,game_launches=0,original_code_executed=False,adapter_apis=['bh_model_draw_lit_context_init','BHModelDrawLitContext prepared/directional/local-direction/global scalar binding','Native synchronous emission callback inherited from unlit draw context','Nonfinite/out-of-signed32 conversion guards'],limits=['Focused source contract target binds isolated capture; parent real native composed scene and current full integration required before admission','Whole0046e390 software dispatcher/rasterizers remain unfinished','PC53/nearest/masked only; no byte equivalence/original differential execution','Prepared light constructor/list lifetime are borrowed; type1prepared_type metadata must be initialized by its owner because prepare intentionally preserves it','Configuration/model/world-to-model changes require context rebind to refresh local directional vector, as original wrapper did per model draw','Face/vertex integer epoch behavior inherited from frozen unlit owners; no fabricated persistent node color cache'])
(FOLDER/'review.json').write_text(json.dumps(review,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(passed=True,complete_source_candidates=1,instructions_verified=len(rows),focused_checks=43338,production_binding_admitted=False,original_code_executed=False,immutable_references_verified=True)))
