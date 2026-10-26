import hashlib,json,struct,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];data=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
assert hashlib.sha256(data).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',data,0x3c)[0];count=struct.unpack_from('<H',data,pe+6)[0];opts=struct.unpack_from('<H',data,pe+20)[0]
def raw(va,size):
 for i in range(count):
  off=pe+24+opts+i*40;_,start,length,pos=struct.unpack_from('<IIII',data,off+8);rva=va-0x400000
  if start<=rva and rva+size<=start+length:return data[pos+rva-start:pos+rva-start+size]
 raise ValueError(hex(va))
owners=[]
for address,name,size,instructions in [('00403120','bh_script_debris_create',451,124),('00402d20','bh_script_debris_tick',242,81),('004027f0','bh_script_debris_refresh',58,18)]:
 va=int(address,16);body=raw(va,size);assert body[-1]==0xc3
 (HERE/(address+'.asm.txt')).write_text(subprocess.check_output(['objdump','-d','-Mintel',f'--start-address=0x{va:x}',f'--stop-address=0x{va+size:x}','GAME_DATA/BHUNTER.EXE'],cwd=ROOT,text=True))
 owners.append(dict(original_va=address,function=name,source='src/bh_script_debris.c',role='engine_candidate',
 provenance_confidence='Whole ordinary source effect constructor and installed lifetime/physics/billboard methods; actual allocator/pool/random/math/draw closure, distinct RET bodies, not CRT/EH/import/shared tails; historical authorship unresolved',
 boundary=dict(start=address,end_exclusive=f'{va+size:08x}',bytes=size,instructions=instructions,sha256=hashlib.sha256(body).hexdigest()),implementation='func',integration='standalone_linked_pending_parent_integration',target='build/script-debris/bhunter-script-debris-check.exe',
 validation=dict(abi='Pinned x86 cdecl entity constructor/step/refresh; actual pool and billboard queue, velocity+48/+4c/+50, verified lifetime/effect scalar fields and persistent private source context, PC53 nearest/masked',
 fixtures=dict(scenario='script-debris',checks=142,failures=0,play_calls=0),original_code_executed=False,game_executed=False,raw_byte_match=False,
 limits=['Full explosion00402e50 remains unadmitted because its00402af0 tick depends on missing004032f0 and its00402710 refresh; already admitted query00402e20 is not counted again.',
         'Borrowed image identity and opaque attribution word004dfb34 are explicit caller bindings; full startup/resource/VM ownership remains incomplete.',
         'Count0 resets attribution without using point; source guards reject negative/count>65535 rather than allowing original effectively unbounded allocation loop.',
         'PC53 seeded random/scalar/physics/queue fixtures; original PC24 transitions, extreme/signal-NaN payloads and original debris artwork remain unvalidated.',
         'No game/original code, gameplay automation, input/UI capture, audio playback or network operation.']),
 evidence=['META/m4/hour424/a2/README.md','META/m4/hour424/a2/standalone-build.json','META/m4/hour424/a2/constants.json','META/m4/hour424/a2/'+address+'.asm.txt']))
(HERE/'owners.json').write_text(json.dumps(owners,indent=2)+'\n')
constants=[]
for va in [0x4891f8,0x489220,0x489228,0x489250,0x489270,0x489278,0x489280,0x4892a0,0x4892e0,0x4892e8,0x4892f0]:
 b=raw(va,8);constants.append(dict(original_va=f'{va:08x}',bytes=b.hex(),double_value=struct.unpack('<d',b)[0]))
(HERE/'constants.json').write_text(json.dumps(constants,indent=2)+'\n')
