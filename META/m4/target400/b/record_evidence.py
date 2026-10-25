import csv,hashlib,json,struct,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
assert hashlib.sha256(exe).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',exe,0x3c)[0];count=struct.unpack_from('<H',exe,pe+6)[0];opts=struct.unpack_from('<H',exe,pe+20)[0];sections=[]
for i in range(count):
 off=pe+24+opts+i*40;vs,va,rs,rp=struct.unpack_from('<IIII',exe,off+8);sections.append((va,vs,rs,rp))
def raw(va,size):
 rva=va-0x400000
 for start,vs,rs,rp in sections:
  if start<=rva and rva+size<=start+rs:return exe[rp+rva-start:rp+rva-start+size]
 raise ValueError(hex(va))
owners=[('00442ec0','bh_script_unloaded_query'),('00443350','bh_script_sound_expiry'),('00443e80','bh_script_motion_query'),('00443f00','bh_script_sensor_query'),('00443f50','bh_script_sound_query'),('00443f90','bh_script_sound_actor_query'),('00437a30','bh_mission_sound_tick'),('00437a80','bh_mission_sound_first'),('00437ab0','bh_mission_sound_second')]
rows={r['address']:r for r in csv.DictReader((ROOT/'analysis/function-map.tsv').open(),delimiter='\t')};result=[]
for address,name in owners:
 row=rows[address];size=int(row['body_bytes']);va=int(address,16)
 asm=subprocess.check_output(['objdump','-d','-Mintel',f'--start-address=0x{va:x}',f'--stop-address=0x{va+size:x}','GAME_DATA/BHUNTER.EXE'],cwd=ROOT,text=True)
 (HERE/(address+'.asm.txt')).write_text(asm)
 assert raw(va,size)[-1]==0xc3
 result.append(dict(original_va=address,function=name,role='engine_candidate',provenance_confidence='Whole entity/script behavior, callsites and field offsets verified; proprietary engine/game authorship unresolved; no CRT/import/compiler support or standalone shared tails',source='src/bh_script_lifetime.c',implementation='func',integration='standalone_linked_pending_parent_integration',boundary=dict(start=address,end_exclusive=f'{va+size:08x}',bytes=size,instructions=int(row['instructions']),sha256=hashlib.sha256(raw(va,size)).hexdigest()),calls=row['calls'],raw_byte_match=False,original_code_executed=False))
(HERE/'owners.json').write_text(json.dumps(result,indent=2)+'\n')
