import csv,hashlib,json,struct,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
data=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
assert hashlib.sha256(data).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',data,0x3c)[0];count=struct.unpack_from('<H',data,pe+6)[0];opts=struct.unpack_from('<H',data,pe+20)[0];sections=[]
for i in range(count):
 off=pe+24+opts+i*40;vs,va,rs,rp=struct.unpack_from('<IIII',data,off+8);sections.append((va,rs,rp))
def raw(va,size):
 rva=va-0x400000
 for start,length,pos in sections:
  if start<=rva and rva+size<=start+length:return data[pos+rva-start:pos+rva-start+size]
 raise ValueError(hex(va))
def asm(start,end):
 return subprocess.check_output(['objdump','-d','-Mintel',f'--start-address=0x{start:x}',f'--stop-address=0x{end:x}','GAME_DATA/BHUNTER.EXE'],cwd=ROOT,text=True)
rows={r['address']:r for r in csv.DictReader((ROOT/'analysis/function-map.tsv').open(),delimiter='\t')};owners=[]
for address,name in [('00443ac0','bh_script_proximity_tick'),('00443d40','bh_script_damage_tick')]:
 row=rows[address];va=int(address,16);size=int(row['body_bytes']);body=raw(va,size)
 (HERE/(address+'.asm.txt')).write_text(asm(va,va+size));assert body[-1]==0xc3
 owners.append(dict(original_va=address,function=name,source='src/bh_script_sensor.c',role='engine_candidate',
  provenance_confidence='Complete ordinary VM-installed sensor step, real entity query/release, native field access and reviewed separate returns; not verified CRT/import/compiler support or independently counted shared tail; game/engine authorship unresolved',
  boundary=dict(start=address,end_exclusive=f'{va+size:08x}',bytes=size,instructions=int(row['instructions']),sha256=hashlib.sha256(body).hexdigest()),
  implementation='func',integration='standalone_linked_pending_parent_integration',target='build/script-sensor/bhunter-script-sensor-check.exe',
  validation=dict(abi='Pinned x86 cdecl BHEntityStep; original+f0/f4/fc/100/104/108/128/12c fields, actual actor148 damage and owner bounds; persistent explicit contexts, PC53 nearest/masked',
   fixtures=dict(scenario='script-sensor',checks=75,failures=0,play_calls=0),original_code_executed=False,game_executed=False,raw_byte_match=False,
   limits=['Count-zero explicit sensor state/context binding; full VM00443ff0, radar list construction, target construction and parser remain unadmitted.',
    'Caller owns stable context, radar and real actor/owner table/pool entity lifetimes. Source rejects malformed binding and stops on current generation/actor mutation after broadcast instead of writing through freed/recycled state.',
    'Original proximity owner releases on final finite shot then reaches a stale-state end-of-list write; source returns after actual release, avoiding that write without changing live behavior.',
    'Event14 packet event/actor/expected and event17 event/actor are contractual; source zeros original unspecified stack remainder and claims no wire protocol.',
    'Actual query owner starts from latest query registration then follows active-next as original; fixtures construct receivers before the sensor and retain this original ordering behavior.',
    'PC53 finite moderate geometry, explicit unordered branch fixtures and pre-store damage threshold checks; original runtime PC24 transitions and NaN payload/sign are unvalidated.',
    'No game/original code, gameplay automation, physical input, native audio playback, UI or network operations.']),
  evidence=['META/m4/hour407/b/'+address+'.asm.txt','META/m4/hour407/b/constants.json','META/m4/hour407/b/parser-install.asm.txt','META/m4/hour407/b/README.md','META/m4/hour407/b/standalone-build.json']))
(HERE/'owners.json').write_text(json.dumps(owners,indent=2)+'\n')
constants=[]
for address,size in [(0x4891f8,8),(0x489218,8),(0x4896f0,8),(0x4895a8,8),(0x489990,4)]:
 b=raw(address,size);constants.append(dict(address=f'{address:08x}',bytes=b.hex(),value=struct.unpack('<d' if size==8 else '<f',b)[0]))
(HERE/'constants.json').write_text(json.dumps(constants,indent=2)+'\n')
(HERE/'parser-install.asm.txt').write_text(asm(0x444c80,0x444d30)+asm(0x445170,0x4452e0))
