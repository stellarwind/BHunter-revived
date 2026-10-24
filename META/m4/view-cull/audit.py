"""Verify sphere mask instruction evidence and source target binding."""
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
at=0x45a7c0;body=bytearray();rows=list(csv.DictReader((FOLDER/'original/0045a7c0.tsv').open(),delimiter='\t'))
for row in rows:
    data=bytes.fromhex(row['bytes']);assert int(row['address'],16)==at and original(at,len(data))==data
    body.extend(data);at+=len(data)
assert at==0x45aa55 and len(rows)==217 and len(body)==661
assert not any(row['instruction'].startswith('CALL ') for row in rows)
assert [row['instruction'] for row in rows if row['instruction'].startswith('AND ')]==['AND ECX,0x3e','AND ECX,0x3d']
assert [row['instruction'] for row in rows if row['instruction'].startswith('SUB ECX,')]==['SUB ECX,0x4','SUB ECX,0x8','SUB ECX,0x10','SUB ECX,0x20']
assert any(row['instruction']=='FST float ptr [EAX]' for row in rows)
caller=list(csv.DictReader((FOLDER/'original/00477880.tsv').open(),delimiter='\t'))
assert sum(row['instruction']=='CALL 0x0045a7c0' for row in caller)==1
for row in caller:
    data=bytes.fromhex(row['bytes']);assert original(int(row['address'],16),len(data))==data
build=json.loads((FOLDER/'focused-build.json').read_text());assert build['passed'] and build['scenario']['checks']==245856 and not build['scenario']['failures']
for source,digest in build['sources'].items():assert sha(ROOT/source)==digest,source
assert build['commands'][-1]['exit_code'] and 'LNK2001' in build['commands'][-1]['output']
for entry in json.loads((ROOT/'META/reference-manifest.json').read_text())['files']:
    if entry['root']=='repo':assert sha(ROOT/entry['path'])==entry['sha256']
record=dict(address='0045a7c0',boundary='0045a7c0-0045aa54',bytes=len(body),instructions=len(rows),body_sha256=hashlib.sha256(body).hexdigest(),role='engine_candidate',provenance_confidence='medium: application sphere/frustum hierarchical mask classification called by recursive rendering owner; no identified reusable library/CRT/EH/import signature',boundary_review='complete contiguous217instructions, ordinary independent multiple-return owner, no outgoing calls or shared-tail assignment',api='bh_view_cull_sphere',source='src/bh_view_cull.c',implementation='func',integration='linked_harness',target='bhunter-view-cull.exe',validation='focused-build.json',game_admitted=False)
review=dict(records=[record],original_exe_sha256=sha(ROOT/'GAME_DATA/BHUNTER.EXE'),sources=build['sources'],complete_original_owners=1,focused_checks=245856,game_launches=0,original_code_executed=False,limits=['Explicit existing BHMainCamera/BHViewConfig bindings are context adapters rather than scattered original-global ABI layout','Finite inputs and valid pointer storage, nonnegative radius, normalized original plane meanings and PC53/nearest/masked are caller contracts; no guards invent new classification','No byte equivalence/original differential execution; PC24/exceptional values remain unvalidated','Unknown mask bits preserved unless an exact depth AND executes; no normalization to six bits; reserved plane1 ignored','Recursive caller00477880 remains unrecovered; source geometry culling is not world traversal/gameplay admission'])
(FOLDER/'review.json').write_text(json.dumps(review,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(passed=True,complete_functions=1,instructions_verified=len(rows),focused_checks=245856,original_code_executed=False,immutable_references_verified=True)))
