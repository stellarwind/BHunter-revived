"""Verify complete attributed clipper body and source target binding."""
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
at=0x453c30;body=bytearray();rows=list(csv.DictReader((FOLDER/'original/00453c30.tsv').open(),delimiter='\t'))
for row in rows:
    data=bytes.fromhex(row['bytes']);assert int(row['address'],16)==at and original(at,len(data))==data
    body.extend(data);at+=len(data)
assert at==0x454fb6 and len(rows)==1656 and len(body)==4998
assert not any(row['instruction'].startswith('CALL ') for row in rows)
assert sum(row['instruction']=='FST float ptr [ESP + 0x10]' for row in rows)==8
assert struct.unpack('<d',original(0x489ee0,8))[0]==1
assert struct.unpack('<d',original(0x489f08,8))[0]==-1
for address,count in [('0045db80',8),('00455f70',7)]:
    caller=list(csv.DictReader((FOLDER/'original'/f'{address}.tsv').open(),delimiter='\t'))
    assert sum(row['instruction']=='CALL 0x00453c30' for row in caller)==count
    for row in caller:
        data=bytes.fromhex(row['bytes']);assert original(int(row['address'],16),len(data))==data
build=json.loads((FOLDER/'focused-build.json').read_text());assert build['passed'] and build['scenario']['checks']==15931 and not build['scenario']['failures']
for source,digest in build['sources'].items():assert sha(ROOT/source)==digest,source
assert build['commands'][-1]['exit_code'] and 'LNK2001' in build['commands'][-1]['output']
for entry in json.loads((ROOT/'META/reference-manifest.json').read_text())['files']:
    if entry['root']=='repo':assert sha(ROOT/entry['path'])==entry['sha256']
record=dict(address='00453c30',boundary='00453c30-00454fb5',bytes=len(body),instructions=len(rows),body_sha256=hashlib.sha256(body).hexdigest(),role='engine_candidate',provenance_confidence='medium: ordinary application geometry owner with two real rendering producers; no identified reusable library/CRT/EH/import signature',boundary_review='complete contiguous1656instructions through RET, independent owner with no outgoing calls and no shared tail',api='bh_attribute_clip_polygon',source='src/bh_attribute_clipper.c',implementation='func',integration='linked_harness',target='bhunter-attribute-clipper.exe',validation='focused-build.json',game_admitted=False)
review=dict(records=[record],adapter_apis=['bh_attribute_clip_init','BHAttributeClipContext original-global/scratch binding'],original_exe_sha256=sha(ROOT/'GAME_DATA/BHUNTER.EXE'),sources=build['sources'],constants_verified=['00489ee0 double+1','00489f08 double-1'],complete_original_owners=1,focused_checks=15931,game_launches=0,original_code_executed=False,limits=['PC53/nearest/masked only; no byte equivalence/original differential execution','Scalar offset28 interpreted as FLOAT from complete FMUL/FSTP instruction evidence; color/alpha/intensity semantics deliberately unresolved','project_depth zero skips near/far and projection, but side cuts still need reciprocalZ and perspectiveUV','Convex ring and valid pointer storage are caller contracts; finite/topology/count/error guards are explicit adapters','Scratch shared globally among original clippers; explicit contexts do not authorize relying on retained output across another clipping operation','Producers0045db80 and00455f70 are evidence dependencies, not recovered here'])
(FOLDER/'review.json').write_text(json.dumps(review,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(passed=True,complete_functions=1,instructions_verified=len(rows),focused_checks=15931,original_code_executed=False,immutable_references_verified=True)))
