import json,hashlib,subprocess,re,struct
from pathlib import Path
exec(Path('META/m4/target500/capture_parent.py').read_text().split('for folder,specs')[0])
here=Path('META/m4/target600/c3');(here/'original').mkdir(parents=True,exist_ok=True)
a=0x415530;n=3042;rows=json.loads(Path('META/m4/target600/c/original/00415530.pre-disasm.json').read_text());body=raw(a,n);assert b''.join(bytes.fromhex(r['bytes']) for r in rows)==body
(here/'original/00415530.disasm.json').write_text(json.dumps(rows,indent=2)+'\n');(here/'original/00415530.decompile.json').write_bytes(Path('META/m4/target600/c/original/00415530.decompile.json').read_bytes())
item=dict(original_va='00415530',function='bh_player_force_control_b',source='src/bh_player_force_control.c',role='game',provenance_confidence='Whole source player forcecontrol variant with real basis solver/sample application and complete plume visual tail. Own RET boundary, no imports/CRT/EH/shared tail.',boundary=dict(start='00415530',end_exclusive=f'{a+n:08x}',bytes=n,instructions=len(rows),sha256=hashlib.sha256(body).hexdigest()))
(here/'owners.json').write_text(json.dumps([item],indent=2)+'\n')
