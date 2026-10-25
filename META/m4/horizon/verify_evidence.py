"""Read-only immutable-PE body/constant checks; record current source fixtures."""
import hashlib,json,struct
from pathlib import Path
root=Path(__file__).resolve().parents[3]
here=Path(__file__).resolve().parent
exe=(root/'GAME_DATA/BHUNTER.EXE').read_bytes()
assert hashlib.sha256(exe).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',exe,60)[0]
table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
sections=[struct.unpack_from('<IIII',exe,table+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
def raw(va,size):
 rva=va-0x400000
 _,start,_,offset=next(s for s in sections if s[1]<=rva and rva+size<=s[1]+s[2])
 return exe[offset+rva-start:offset+rva-start+size]
contracts={
 '004784c0':(116,'bh_horizon_queue','src/bh_horizon_queue.c','header-zero sparse horizon command producer'),
 '00477aa0':(52,'bh_horizon_queue_execute','src/bh_horizon_queue.c','mode0 barrier/full horizon/next-link callback'),
 '0045db80':(2958,'bh_horizon_draw','src/bh_horizon.c','all nine vertices/eight clipped polygons/depth/fog/color/flush owner'),
 '0046d630':(925,'bh_depth_subdivide','src/bh_depth_subdivide.c','plain recursive depth subdivision with selected leaf dependency'),
 '0046d9d0':(974,'bh_depth_subdivide_scalar','src/bh_depth_subdivide.c','scalar recursive depth subdivision with selected leaf dependency'),
}
owners=[]
for address,(size,symbol,source,contract) in contracts.items():
 rows=json.loads((here/'original'/(address+'.disasm.json')).read_text(encoding='utf-8-sig'))
 at=int(address,16);body=bytearray()
 for row in rows:
  assert int(row['address'],16)==at,(address,row)
  b=bytes.fromhex(row['bytes']);body.extend(b);at+=len(b)
 assert len(body)==size and rows[-1]['mnemonic']=='RET' and body==raw(int(address,16),size)
 owners.append(dict(address=address,end_inclusive=f'{at-1:08x}',bytes=size,instructions=len(rows),body_sha256=hashlib.sha256(body).hexdigest(),role='engine_candidate',provenance_confidence='high engine role; engine attribution unknown',implementation='func',source=source,symbol=symbol,contract=contract,integration='focused linked source harness; parent integration admission separately required'))
constants={}
for address,size in [('00489f80',4),('00489ec8',8),('00489f18',4),('0049175c',4),('00489ee0',8),('00491770',4),('00491774',4)]:
 b=raw(int(address,16),size);constants[address]=dict(bytes=b.hex(),value=struct.unpack('<f' if size==4 else '<d',b)[0])
constants['0049175c']['storage']='mutable near_z; initialized PE value only;00451720 assigns argument2'
sources=['src/bh_horizon.c','src/bh_horizon.h','src/bh_horizon_queue.c','src/bh_horizon_queue.h','src/bh_depth_subdivide.c','src/bh_depth_subdivide.h','tests/horizon_queue.c','tests/depth_subdivide.c','tests/depth_subdivide_native.c','tests/horizon.c','tests/horizon_native.c']
results={}
for name in ('standalone','depth-standalone','depth-native','horizon-standalone','horizon-native'):
 data=json.loads((here/(name+'.json')).read_text());assert data['passed'],name
 for path,digest in data['source_sha256'].items():assert hashlib.sha256((root/path).read_bytes()).hexdigest()==digest,(name,path)
 results[name]=dict(passed=True,output=data['commands'][-1]['stdout'])
evidence=dict(schema_version=1,owners=owners,constants=constants,source_sha256={s:hashlib.sha256((root/s).read_bytes()).hexdigest() for s in sources},focused_results=results,original_code_executed=False,game_launches=0,not_claimed=['0046e530 whole native/software dispatcher','0046dda0 software lock/dispatch/raster integration','any handwritten raster kernel','software pixel rendering from recording fixtures','original runtime/PC24/huge-angle equivalence','game target admission'],native_adapter=dict(symbol='bh_horizon_native_emit',source='src/bh_horizon.c',implementation='adapter',contract='native-only0046e530 live-near-z TL/fog scalar composition through real source queue; original software branch explicitly rejected'))
(here/'evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
print(json.dumps(dict(owners=len(owners),complete_instruction_records=sum(o['instructions'] for o in owners),focused_targets=len(results),original_code_executed=False,game_launches=0)))
