import json,hashlib,subprocess,struct
from pathlib import Path
exec(Path('META/m4/target500/capture_parent.py').read_text().split('for folder,specs')[0])
here=Path('META/m4/target600/c5');(here/'original').mkdir(parents=True,exist_ok=True);a=0x428dc0;n=391
rows=json.loads(subprocess.check_output(['ghidra','--project','bhunter-frame','disasm',hex(a),'-n','160'],text=True));rows=[r for r in rows if a<=int(r['address'],16)<a+n];body=raw(a,n);assert b''.join(bytes.fromhex(r['bytes']) for r in rows)==body
(here/'original/00428dc0.disasm.json').write_text(json.dumps(rows,indent=2)+'\n');item=dict(original_va='00428dc0',function='bh_network_debug_draw',source='src/bh_network_debug.c',role='game',provenance_confidence='Complete network debug HUD orchestration owner over real GDI queue, packet counters and actual player roster. CRT sprintf reused; no import/helper/shared tail.',boundary=dict(start='00428dc0',end_exclusive=f'{a+n:08x}',bytes=n,instructions=len(rows),sha256=hashlib.sha256(body).hexdigest()));(here/'owners.json').write_text(json.dumps([item],indent=2)+'\n')
for a in [0x4903a8,0x4903c0,0x4903d0,0x4903e0,0x4903f0,0x490408]:print(hex(a),raw(a,40).split(b'\0')[0])
print('scale',struct.unpack('<d',raw(0x489ad0,8)))
