import json,hashlib,subprocess
from pathlib import Path
exec(Path('META/m4/target500/capture_parent.py').read_text().split('for folder,specs')[0])
here=Path('META/m4/target550/c');owners=[]
for address,n,name in [('004363f0',633,'bh_script_actor_curve')]:
 rows=json.loads(subprocess.check_output(['ghidra','--project','bhunter-frame','disasm',address,'-n',str(n)],text=True));a=int(address,16);rows=[r for r in rows if a<=int(r['address'],16)<a+n];body=raw(a,n)
 assert b''.join(bytes.fromhex(r['bytes']) for r in rows)==body and rows[-1]['mnemonic']=='RET'
 (here/'original'/(address+'.disasm.json')).write_text(json.dumps(rows,indent=2)+'\n')
 dec=subprocess.check_output(['ghidra','--project','bhunter-frame','decompile',address],text=True);(here/'original'/(address+'.decompile.json')).write_text(json.dumps(json.loads(dec),indent=2)+'\n')
 owners.append(dict(original_va=address,function=name,source='src/bh_script_actor_path.c',role='engine_candidate',provenance_confidence='Complete indirect route interpolation owner installed by36fa0; distinct RET boundary. No CRT, compiler helper, import or shared tail; original authorship unresolved.',boundary=dict(start=address,end_exclusive=f'{a+n:08x}',bytes=n,instructions=len(rows),sha256=hashlib.sha256(body).hexdigest()),validation=dict(abi='x86 cdecl6argument route callback. Borrowed BHVec3 input/output; no globals or owned storage.',limits=['PC53 nearest/masked moderate finite control vectors and phase. PC24 remains unresolved. CRT sin/cos may differ from original x87 FSIN/FCOS at last bits; no raw or differential match claimed.','Analytic interior1000sample sweep and exact endpoint/extrapolation, output alias fixtures. Original code, game, startup and gameplay not executed.','Real topology, draw, ticks, query and43bec0constructor are independently recovered in c2..c6.43a920/43bca0 remain open.36670is an already-admitted owner reused through a count-zero wrapper, excluded here.']),evidence=['META/m4/target550/c/standalone-build.json',f'META/m4/target550/c/original/{address}.disasm.json']))
(here/'owners.json').write_text(json.dumps(owners,indent=2)+'\n')
