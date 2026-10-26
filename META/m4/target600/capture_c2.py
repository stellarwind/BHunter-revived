import json,hashlib,subprocess
from pathlib import Path
exec(Path('META/m4/target500/capture_parent.py').read_text().split('for folder,specs')[0])
here=Path('META/m4/target600/c2');(here/'original').mkdir(parents=True,exist_ok=True);items=[]
for address,size,name in [('00426eb0',216,'bh_network_scores_broadcast'),('00426f90',484,'bh_network_scores_death')]:
 a=int(address,16);rows=json.loads(subprocess.check_output(['ghidra','--project','bhunter-frame','disasm',address,'-n',str(size)],text=True));rows=[r for r in rows if a<=int(r['address'],16)<a+size];body=raw(a,size);assert b''.join(bytes.fromhex(r['bytes']) for r in rows)==body and rows[-1]['mnemonic']=='RET'
 (here/'original'/(address+'.disasm.json')).write_text(json.dumps(rows,indent=2)+'\n');(here/'original'/(address+'.decompile.json')).write_text(subprocess.check_output(['ghidra','--project','bhunter-frame','decompile',address],text=True))
 items.append(dict(original_va=address,function=name,source='src/bh_network_scores.c',role='game',provenance_confidence='Whole native scorepacket orchestration and score mutation owners; callbacks use actual roster, actual text producer275c0 and native transport. Distinct RET boundaries; no CRT/EH/import/shared tail.',boundary=dict(start=address,end_exclusive=f'{a+size:08x}',bytes=size,instructions=len(rows),sha256=hashlib.sha256(body).hexdigest())))
(here/'owners.json').write_text(json.dumps(items,indent=2)+'\n')
