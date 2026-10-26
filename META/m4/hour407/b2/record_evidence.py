import csv,hashlib,json,struct,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
data=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
assert hashlib.sha256(data).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',data,0x3c)[0];count=struct.unpack_from('<H',data,pe+6)[0];opts=struct.unpack_from('<H',data,pe+20)[0];sections=[]
for i in range(count):
 off=pe+24+opts+i*40;vs,va,rs,rp=struct.unpack_from('<IIII',data,off+8);sections.append((va,rs,rp))
va=0x455840;size=129;rva=va-0x400000
body=next(data[pos+rva-start:pos+rva-start+size] for start,length,pos in sections if start<=rva and rva+size<=start+length)
assert body[-1]==0xc3
asm=subprocess.check_output(['objdump','-d','-Mintel','--start-address=0x455840','--stop-address=0x4558c1','GAME_DATA/BHUNTER.EXE'],cwd=ROOT,text=True)
(HERE/'00455840.asm.txt').write_text(asm)
owner=dict(original_va='00455840',function='bh_project_point_scale',source='src/bh_project_scale.c',role='engine_candidate',
 provenance_confidence='Whole ordinary projected camera helper, direct real matrix point-transform call and distinct near reject/success returns; not verified CRT/import/compiler helper/shared tail; proprietary engine/game authorship unresolved',
 boundary=dict(start='00455840',end_exclusive='004558c1',bytes=129,instructions=46,sha256=hashlib.sha256(body).hexdigest()),
 implementation='func',integration='standalone_linked_pending_parent_integration',target='build/project-scale/bhunter-project-scale-check.exe',
 validation=dict(abi='Pinned32-bit x86 cdecl; original in-place world scalars copied before screenX/screenY/scale output stores; actual source BHMainCamera projected matrix/focal and BHViewConfig near plane replace scattered globals; PC53 nearest/masked',
  fixtures=dict(scenario='project-scale',checks=21,failures=0),original_code_executed=False,game_executed=False,raw_byte_match=False,
  limits=['Whole helper only; target draw0043c3f0 and target actor construction remain unadmitted. Adjacent004558d0 already admitted and is not counted again.',
   'Persistent borrowed camera/view and disjoint camera/config versus scalar outputs; scalar pointers may alias each other with original sequential store behavior.',
   'Source pointer and FP-profile guards reject before input mutation; no global camera initialization or original binary ABI drop-in claim.',
   'Moderate finite camera geometry, masked zero-depth division and unordered near/depth class fixtures; NaN payload/sign, runtimePC24 and general extreme matrix numerics remain unvalidated.',
   'No renderer callback or native backend is substituted: direct actual math definitions and source camera builder are linked. No game/original code, input, automation or playback.']),
 evidence=['META/m4/hour407/b2/00455840.asm.txt','META/m4/hour407/b2/README.md','META/m4/hour407/b2/standalone-build.json'])
(HERE/'owners.json').write_text(json.dumps([owner],indent=2)+'\n')
