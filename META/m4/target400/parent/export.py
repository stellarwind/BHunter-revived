import csv,json,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[4];sys.path.insert(0,str(ROOT/'tools'))
from record_pickup_basic import reference
HERE=Path(__file__).resolve().parent
specs=[('00402e20','bh_actor_target_removed'),('00407180','bh_effect_target_removed'),('00407b00','bh_marker_query'),('00442330','bh_pickup_target_removed'),('00436670','bh_vec3_interpolate'),('00423a10','bh_packet_read_vector')]
base=['ghidra','--projects-dir',(ROOT/'ghidra-projects').as_posix(),'--project','Bhunter','--program','BHUNTER.EXE']
mapping={r['address']:r for r in csv.DictReader((ROOT/'analysis/function-map.tsv').open(),delimiter='\t')}
ledger,raw=reference();owners=[]
import hashlib,struct
assert raw(0x489310,8)==struct.pack('<d',1.0/128)
for address,function in specs:
 m=mapping[address];rows=json.loads(subprocess.check_output(base+['disasm','0x'+address,'-n',m['instructions']],text=True));at=int(address,16);body=bytearray()
 for r in rows:
  assert int(r['address'],16)==at;b=bytes.fromhex(r['bytes']);assert b==raw(at,len(b));body.extend(b);at+=len(b)
 assert len(body)==int(m['body_bytes']) and rows[-1]['mnemonic']=='RET'
 (HERE/(address+'.disasm.json')).write_text(json.dumps(rows,indent=2)+'\n')
 refs=subprocess.run(base+['x-ref','to','0x'+address],capture_output=True,text=True);(HERE/(address+'.xrefs.json')).write_text(refs.stdout)
 owners.append(dict(address=address,function=function,source='src/bh_callback_helpers.c',role='engine_candidate',scenarios=['callback-helpers'],ranges=[dict(start=address,end_exclusive=f'{at:08x}',bytes=len(body),instructions=len(rows),sha256=hashlib.sha256(body).hexdigest())],original=(HERE/(address+'.disasm.json')).relative_to(ROOT).as_posix()))
(HERE/'owners.json').write_text(json.dumps(owners,indent=2)+'\n')
print('Verified six whole PE bodies and scale constant; read-only code/data.')

