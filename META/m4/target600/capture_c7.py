import json,hashlib,subprocess
from pathlib import Path
exec(Path('META/m4/target500/capture_parent.py').read_text().split('for folder,specs')[0])
here=Path('META/m4/target600/c7');(here/'original').mkdir(parents=True,exist_ok=True);a=0x417c50;n=735
rows=json.loads(subprocess.check_output(['ghidra','--project','bhunter-frame','disasm',hex(a),'-n','260'],text=True));rows=[r for r in rows if a<=int(r['address'],16)<a+n];body=raw(a,n);assert b''.join(bytes.fromhex(r['bytes']) for r in rows)==body
(here/'original/00417c50.disasm.json').write_text(json.dumps(rows,indent=2)+'\n');item=dict(original_va='00417c50',function='bh_player_aim_update',source='src/bh_player_aim.c',role='game',provenance_confidence='Whole player aim shaping/basis projection owner; independent multipleRET body, CIpow verified CRT reuse excluded. No imports/EH/sharedtail.',boundary=dict(start='00417c50',end_exclusive=f'{a+n:08x}',bytes=n,instructions=len(rows),sha256=hashlib.sha256(body).hexdigest()));(here/'owners.json').write_text(json.dumps([item],indent=2)+'\n')
