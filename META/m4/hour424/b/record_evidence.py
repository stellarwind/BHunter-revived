import csv,hashlib,json,struct,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
data=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert hashlib.sha256(data).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',data,0x3c)[0];n=struct.unpack_from('<H',data,pe+6)[0];opts=struct.unpack_from('<H',data,pe+20)[0];sections=[]
for i in range(n):
 p=pe+24+opts+i*40;vs,va,rs,rp=struct.unpack_from('<IIII',data,p+8);sections.append((va,rs,rp))
def raw(va,size):
 rva=va-0x400000
 for start,length,pos in sections:
  if start<=rva and rva+size<=start+length:return data[pos+rva-start:pos+rva-start+size]
 raise ValueError(hex(va))
def asm(a,b):return subprocess.check_output(['objdump','-d','-Mintel',f'--start-address=0x{a:x}',f'--stop-address=0x{b:x}','GAME_DATA/BHUNTER.EXE'],cwd=ROOT,text=True)
body=raw(0x43c3f0,1215);assert body[-1]==0xc3
(HERE/'0043c3f0.asm.txt').write_text(asm(0x43c3f0,0x43c8af))
(HERE/'constructor-pointers.asm.txt').write_text(asm(0x43d730,0x43d7c0)+asm(0x43dd30,0x43ddc0)+asm(0x43e590,0x43e620))
ads=[(0x4897e8,8),(0x4894c0,8),(0x48983c,4),(0x48958c,4),(0x4897f0,4),(0x489ce0,4),(0x489ce4,4),(0x489ce8,4),(0x489cec,4),(0x489cf0,4),(0x489cf4,4),(0x4893c8,4),(0x489248,4),(0x489cf8,4),(0x489cfc,4),(0x489d00,4),(0x4893d8,4),(0x489808,4),(0x48984c,4),(0x489848,4),(0x489768,8),(0x489280,8),(0x489310,8)]
constants=[dict(address=f'{a:08x}',bytes=raw(a,n).hex(),value=struct.unpack('<d' if n==8 else '<f',raw(a,n))[0]) for a,n in ads]
(HERE/'constants.json').write_text(json.dumps(constants,indent=2)+'\n')
owner=dict(original_va='0043c3f0',function='bh_target_draw',source='src/bh_target_draw.c',role='engine_candidate',
 provenance_confidence='Whole ordinary target-class model/numeric/marker queue behavior; real constructor immediate references at0043d770/0043dd69/0043e5c0 and direct admitted producer/projector dependencies. Not verified CRT/import/compiler support/shared tail; historical game/proprietary-engine authorship unresolved',
 boundary=dict(start='0043c3f0',end_exclusive='0043c8af',bytes=1215,instructions=374,sha256=hashlib.sha256(body).hexdigest()),
 implementation='func',integration='standalone_linked_pending_parent_integration',target='build/target-draw/bhunter-target-draw-check.exe',
 validation=dict(abi='Pinned x86 cdecl behavior over original target class fields: basis18..38,angles3c..44,anchorf0..f8,number118,flags120,resource124,owner128; explicit camera/view/queue/atlas/marker/player and signed original004e2998 width bindings; PC53 nearest/masked',
  fixtures=dict(scenario='target-draw',checks=88,failures=0,play_calls=0),original_code_executed=False,game_executed=False,raw_byte_match=False,
  native_offscreen='Actual640x480 DirectDraw/Direct3D2 private RGB/depth target, staged synthetic white texture, actual model resource/BSP/cache/clip/native emission and rectangle consumer/clip/native emission, residency/DrawPrimitive, direct owned-surface pixel checks; no UI image or screenshot written.',
  limits=['Whole producer only; target construction/physics/collision owners that install it remain unadmitted; explicit field views/bindings add no count.',
   'Caller supplies persistent actual resource/image handle resolvers, player identity, camera/view/queue and target; no guessed globals, callback stubs or original executable pointers are callable.',
   'Source returns1/allqueued,0/anyexhaustion,-1/binding error while retaining every successful append; original void owner silently ignored producer returns. Positive-width and current FP-profile guards precede queue mutation.',
   'Synthetic triangle/white atlas validate actual source/native execution and every queued geometry/UV branch, not original target art/assets or gameplay selection lifetimes.',
   'Moderate finite PC53 geometry, wrapped LONG_MIN and large signed digit quotient fixtures; PC24 runtime transitions, extreme matrix/NaN payload behavior and software raster dispatch remain open.',
   'No game/original code, automated gameplay, input, UI capture, native audio playback or network operation.']),
 evidence=['META/m4/hour424/b/README.md','META/m4/hour424/b/0043c3f0.asm.txt','META/m4/hour424/b/0043c3f0.decompile.json','META/m4/hour424/b/constants.json','META/m4/hour424/b/constructor-pointers.asm.txt','META/m4/hour424/b/standalone-build.json'])
(HERE/'owners.json').write_text(json.dumps([owner],indent=2)+'\n')
