"""Verify complete instruction exports and bind source-only target evidence."""
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
records=[]
for address,end,api,behavior in [
    ('00451720','00451744','bh_main_camera_settings','FOV degrees converted using original double pi/180; near/far float stores'),
    ('00451750','00451b7a','bh_main_camera_build','Global projection; rigid inverse; eye; forward and four side planes; reserved plane1 untouched; reset projection-state word'),
    ('00452070','0045213e','bh_main_camera_pose','Float pose stores and view matrix constructed from translation then world Y/X/Z; global camera build'),
    ('004018a0','00401998','bh_mat34_rotate_world_z','Pre-rotate world Z including translation; preserve entire depth row')]:
    rows=list(csv.DictReader((FOLDER/'original'/f'{address}.tsv').open(),delimiter='\t'));at=int(address,16);body=bytearray()
    for row in rows:
        data=bytes.fromhex(row['bytes']);assert int(row['address'],16)==at and original(at,len(data))==data
        body.extend(data);at+=len(data)
    assert at-1==int(end,16)
    records.append(dict(address=address,boundary=f'{address}-{end}',bytes=len(body),instructions=len(rows),body_sha256=hashlib.sha256(body).hexdigest(),role='engine_candidate',provenance_confidence='medium: ordinary application camera/math ownership and callers; library authorship unresolved; no verified CRT/import/EH signature',boundary_review='complete contiguous instructions through RET, independent owner, no shared tail',api=api,behavior=behavior,source='src/bh_main_camera.c',implementation='func',integration='linked_harness',target='bhunter-main-camera.exe',validation='focused-build.json',game_admitted=False))
assert struct.unpack('<d',original(0x489ef0,8))[0]==0.017453292519943295
assert struct.unpack('<f',original(0x489ef8,4))[0]==.5
assert struct.unpack('<f',original(0x489efc,4))[0]==-.5
assert struct.unpack('<12f',original(0x48c030,48))==(1,0,0,0,0,1,0,0,0,0,1,0)
build=json.loads((FOLDER/'focused-build.json').read_text())
assert build['passed'] and build['scenario']['checks']==2025 and not build['scenario']['failures']
for source,digest in build['sources'].items():assert sha(ROOT/source)==digest,source
assert build['commands'][-1]['exit_code'] and 'LNK2001' in build['commands'][-1]['output']
for entry in json.loads((ROOT/'META/reference-manifest.json').read_text())['files']:
    if entry['root']=='repo':assert sha(ROOT/entry['path'])==entry['sha256']
review=dict(records=records,original_exe_sha256=sha(ROOT/'GAME_DATA/BHUNTER.EXE'),source_sha256=build['sources']['src/bh_main_camera.c'],header_sha256=build['sources']['src/bh_main_camera.h'],test_sha256=build['sources']['tests/main_camera.c'],constants_verified=['00489ef0 double pi/180','00489ef8 float +0.5','00489efc float -0.5','0048c030 identity matrix'],game_launches=0,original_code_executed=False,limits=['Explicit BHViewConfig/BHMainCamera context binding is an adapter, not assertion that original scattered globals formed these structs','Finite-input guards intentionally reject invalid/overflowing settings or outputs and preserve destination','PC53/nearest/masked profile only; PC24, huge-angle x87 reduction, exceptional NaN payloads and original differential execution unvalidated','No byte match claimed; no gameplay/camera selection/visibility traversal or near/far clipping admission','Input may alias own view or own projected; other partial/output overlap unsupported','Original global plane slot1 is retained without guessing its owner or manufacturing a near/far plane'])
(FOLDER/'review.json').write_text(json.dumps(review,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(passed=True,complete_functions=len(records),instructions_verified=sum(r['instructions'] for r in records),focused_checks=build['scenario']['checks'],original_code_executed=False,immutable_references_verified=True)))
