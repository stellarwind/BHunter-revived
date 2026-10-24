"""Verify original screen clipper instructions and focused source evidence."""
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
at=0x453300;body=bytearray();rows=list(csv.DictReader((FOLDER/'original/00453300.tsv').open(),delimiter='\t'))
for row in rows:
    data=bytes.fromhex(row['bytes']);assert int(row['address'],16)==at and original(at,len(data))==data
    body.extend(data);at+=len(data)
assert at==0x453c2b and len(rows)==777 and len(body)==2347
assert not any(row['instruction'].startswith('CALL ') for row in rows)
for address in ('00454fc0','00455410','00455960'):
    caller=list(csv.DictReader((FOLDER/'original'/f'{address}.tsv').open(),delimiter='\t'))
    calls=[row for row in caller if row['instruction']=='CALL 0x00453300'];assert len(calls)==1
    for row in caller:
        data=bytes.fromhex(row['bytes']);assert original(int(row['address'],16),len(data))==data
build=json.loads((FOLDER/'focused-build.json').read_text());assert build['passed'] and build['scenario']['checks']==11038 and not build['scenario']['failures']
for source,digest in build['sources'].items():assert sha(ROOT/source)==digest,source
assert build['commands'][-1]['exit_code'] and 'LNK2001' in build['commands'][-1]['output']
for entry in json.loads((ROOT/'META/reference-manifest.json').read_text())['files']:
    if entry['root']=='repo':assert sha(ROOT/entry['path'])==entry['sha256']
record=dict(address='00453300',boundary='00453300-00453c2a',bytes=len(body),instructions=len(rows),body_sha256=hashlib.sha256(body).hexdigest(),role='engine_candidate',provenance_confidence='medium: complete application geometry owner called by sprite producers, no identified reusable library or CRT/EH/import signature',boundary_review='complete contiguous instructions with ordinary independent entry/multiple returns, no outgoing calls, no shared-tail assignment',api='bh_screen_clip_polygon',source='src/bh_screen_clipper.c',implementation='func',integration='linked_harness',target='bhunter-screen-clipper.exe',validation='focused-build.json',game_admitted=False)
review=dict(records=[record],adapter_apis=['bh_screen_clip_init','BHScreenClipContext explicit original-global and scratch binding'],original_exe_sha256=sha(ROOT/'GAME_DATA/BHUNTER.EXE'),sources=build['sources'],complete_original_owners=1,focused_checks=build['scenario']['checks'],game_launches=0,original_code_executed=False,limits=['PC53/nearest/masked only; no byte equivalence or original differential execution','Convex ring and valid pointer storage are caller contracts; malformed-link/nonfinite/bounded-count checks are source guards','Two depth bounds reject the whole uniform-depth polygon; only four screen bounds generate vertices; no fabricated general6plane clipping','Existing input vertices retain their per-vertex depth and cache payload; new scratch writes only original depth/UV/screen/cache/link fields','Original scratch is shared with textured clipper; adapter contexts are explicit caller-owned storage and output must be consumed before any subsequent clipping operation','Sprite producers00454fc0/00455410/00455960 remain unrecovered and are exported for dependency evidence only'])
(FOLDER/'review.json').write_text(json.dumps(review,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(passed=True,complete_functions=1,instructions_verified=len(rows),focused_checks=11038,original_code_executed=False,immutable_references_verified=True)))
