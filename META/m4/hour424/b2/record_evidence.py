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
rows={r['address']:r for r in csv.DictReader((ROOT/'analysis/function-map.tsv').open(),delimiter='\t')};owners=[]
for address,name in [('00440030','bh_projectile_cleanup'),('00440810','bh_projectile_query')]:
 row=rows[address];va=int(address,16);size=int(row['body_bytes']);body=raw(va,size);assert body[-1]==0xc3
 (HERE/(address+'.asm.txt')).write_text(asm(va,va+size))
 owners.append(dict(original_va=address,function=name,source='src/bh_projectile_lifetime.c',role='engine_candidate',
  provenance_confidence='Whole ordinary projectile lifetime/query behavior with verified entry/returns and live cdecl arguments; query event20 installs cleanup00440030, which calls actual sound Stop and pool release; not CRT/import/compiler support/shared tail. Historical game/engine authorship unresolved.',
  boundary=dict(start=address,end_exclusive=f'{va+size:08x}',bytes=size,instructions=int(row['instructions']),sha256=hashlib.sha256(body).hexdigest()),
  implementation='func',integration='standalone_linked_pending_parent_integration',target='build/projectile-lifetime/bhunter-projectile-lifetime-check.exe',
  validation=dict(abi='Pinned x86 cdecl BHEntityStep/query; original e8 float countdown,118 sound handle,11c packet identity,12c/130 borrowed entity references; original event18 packet word+4 mutation and kind+8/position+10; actual real pool/sound owners',
   fixtures=dict(scenario='projectile-lifetime',checks=37,failures=0,play_calls=0),original_code_executed=False,game_executed=False,raw_byte_match=False,
   limits=['Whole lifecycle/query owners only; original projectile construction, motion, physics, refresh rendering and network transport are not admitted by this family.',
    'Count-zero source context binding installs actual query, retaining existing step/refresh until original event20 overrides them. Persistent borrowed pool/sound context and referenced entities must outlive calls; private context/generation at payload468/472.',
    'Source guard rejects foreign/dead/recycled current entities before effects; bound current entity/pool/context must remain valid storage. Sound Stop result remains ignored; invalid/stale handle does not block release or force handle invalidation.',
    'Source pool-only streaming resources validate real request/Stop ownership without playback; hardware/nonstreaming Stop remains the already admitted sound owner, not a new native playback claim.',
    'Quiet NaN countdown class and finite clamp behavior tested; signaling NaN payload/exception ordering and runtime PC24 remain unvalidated.',
    'No game/original code, gameplay automation, physical input, UI capture, native audio playback or network operation.']),
  evidence=['META/m4/hour424/b2/README.md','META/m4/hour424/b2/'+address+'.asm.txt','META/m4/hour424/b2/constants.json','META/m4/hour424/b2/standalone-build.json','META/m4/hour424/b2/PHYSICS_REVIEW.md']))
(HERE/'owners.json').write_text(json.dumps(owners,indent=2)+'\n')
(HERE/'constants.json').write_text(json.dumps([dict(address='00489988',bytes=raw(0x489988,8).hex(),binary64=struct.unpack('<d',raw(0x489988,8))[0]),dict(original_immediate='3e4ccccd',binary32=struct.unpack('<f',bytes.fromhex('cdcc4c3e'))[0])],indent=2)+'\n')
(HERE/'rejected-shared-tail-00437770.asm.txt').write_text(asm(0x437770,0x4377d2))
