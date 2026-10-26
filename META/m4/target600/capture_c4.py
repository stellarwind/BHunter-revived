import json,hashlib,subprocess
from pathlib import Path
exec(Path('META/m4/target500/capture_parent.py').read_text().split('for folder,specs')[0])
here=Path('META/m4/target600/c4');(here/'original').mkdir(parents=True,exist_ok=True);items=[]
for address,n,name in [('00414880',3243,'bh_player_force_control_a'),('00416120',3468,'bh_player_force_control_c'),('00416eb0',3486,'bh_player_force_control_d')]:
 a=int(address,16);rows=json.loads(Path('META/m4/target600/c/original',address+'.pre-disasm.json').read_text());body=raw(a,n);assert b''.join(bytes.fromhex(r['bytes']) for r in rows)==body
 (here/'original'/(address+'.disasm.json')).write_text(json.dumps(rows,indent=2)+'\n')
 items.append(dict(original_va=address,function=name,source='src/bh_player_force_control.c',role='game',provenance_confidence='Complete player stabilizer/force-control owner, with all smoothing and visual endpoints. Distinct RET boundary; no CRT/EH/import/shared tails.',boundary=dict(start=address,end_exclusive=f'{a+n:08x}',bytes=n,instructions=len(rows),sha256=hashlib.sha256(body).hexdigest())))
(here/'owners.json').write_text(json.dumps(items,indent=2)+'\n')
