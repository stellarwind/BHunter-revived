import json,hashlib,subprocess
from pathlib import Path
exec(Path('META/m4/target500/capture_parent.py').read_text().split('for folder,specs')[0])
here=Path('META/m4/target600/c');(here/'original').mkdir(parents=True,exist_ok=True)
items=[]
for address,size,name in [('00413990',738,'bh_actor_basis_direction_pair'),('00413c80',758,'bh_actor_basis_torque_pair'),('004146f0',395,'bh_actor_basis_apply_group')]:
 a=int(address,16);rows=json.loads(subprocess.check_output(['ghidra','--project','bhunter-frame','disasm',address,'-n',str(size)],text=True));rows=[r for r in rows if a<=int(r['address'],16)<a+size];body=raw(a,size)
 assert b''.join(bytes.fromhex(r['bytes']) for r in rows)==body and rows[-1]['mnemonic']=='RET'
 (here/'original'/(address+'.disasm.json')).write_text(json.dumps(rows,indent=2)+'\n')
 dec=json.loads(subprocess.check_output(['ghidra','--project','bhunter-frame','decompile',address],text=True));(here/'original'/(address+'.decompile.json')).write_text(json.dumps(dec,indent=2)+'\n')
 items.append(dict(original_va=address,function=name,source='src/bh_actor_basis_control.c',role='engine_candidate',provenance_confidence='Whole cdecl player force-control arithmetic owner over actual actor-basis groups/samples. No CRT, import, EH or shared tail; historical authorship unresolved.',boundary=dict(start=address,end_exclusive=f'{a+size:08x}',bytes=size,instructions=len(rows),sha256=hashlib.sha256(body).hexdigest()),implementation='wip'))
(here/'owners.json').write_text(json.dumps(items,indent=2)+'\n')
