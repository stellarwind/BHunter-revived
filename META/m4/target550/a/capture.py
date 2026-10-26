import hashlib,json,struct,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
p=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
assert hashlib.sha256(p).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',p,60)[0];s=pe+24+struct.unpack_from('<H',p,pe+20)[0]
sections=[struct.unpack_from('<IIII',p,s+i*40+8) for i in range(struct.unpack_from('<H',p,pe+6)[0])]
def raw(a,n):
 for vs,va,size,offset in sections:
  if va<=a-0x400000<va+size:return p[offset+a-0x400000-va:offset+a-0x400000-va+n]
 raise ValueError(hex(a))
rows=json.loads(subprocess.check_output(['ghidra','--project','bhunter-frame','disasm','0x0041d7c0','-n','1110'],text=True))
body=raw(0x41d7c0,3749)
assert b''.join(bytes.fromhex(r['bytes']) for r in rows)==body
assert rows[-1]['address']=='0041e664' and rows[-1]['mnemonic']=='RET'
original=HERE/'original';original.mkdir(exist_ok=True)
(original/'0041d7c0.disasm.json').write_bytes((json.dumps(rows,indent=2)+'\n').encode())
for kind in ['c','asm']:
 (original/('0041d7c0.'+kind+'.txt')).write_bytes((ROOT/('META/m3/damage-review/0041d7c0.'+kind+'.txt')).read_bytes().replace(b'\r\n',b'\n'))
constants=[]
for address,kind in [(0x489500,'d'),(0x489200,'f'),(0x48930c,'f'),(0x489990,'f'),(0x489220,'d'),(0x489360,'d'),(0x489568,'d'),(0x489998,'d'),(0x4899a0,'d'),(0x489280,'d'),(0x489890,'d'),(0x489300,'d'),(0x4899a8,'d'),(0x4899b0,'d'),(0x4892c0,'f'),(0x4892e8,'d'),(0x489580,'d'),(0x489260,'d'),(0x489210,'d'),(0x489588,'f'),(0x48958c,'f'),(0x489248,'f')]:
 b=raw(address,struct.calcsize(kind));constants.append(dict(address=f'{address:08x}',type=kind,bytes=b.hex(),value=struct.unpack('<'+kind,b)[0]))
data=[]
for address,n,role in [(0x41e668,68,'seventeen switch entries'),(0x48f960,40,'ten pickup refill quantities')]:
 b=raw(address,n);data.append(dict(address=f'{address:08x}',size=n,role=role,bytes=b.hex(),sha256=hashlib.sha256(b).hexdigest()))
strings=[]
for address in [0x48f378,0x48f3e0,0x48f448]:
 b=raw(address,256).split(b'\0')[0];strings.append(dict(address=f'{address:08x}',text=b.decode('ascii'),sha256=hashlib.sha256(b+b'\0').hexdigest()))
(HERE/'constants.json').write_bytes((json.dumps(dict(constants=constants,data=data,strings=strings,owned_data_count=0),indent=2)+'\n').encode())
owner=dict(original_va='0041d7c0',function='bh_player_query_dispatch',source='src/bh_player_query.c',role='game',provenance_confidence='High game behavior: constructor00421d20 installs callback at0042208e; complete event switch owns hunter money/fines/outlaw messages, ten weapon pickups, protection and impact response. Separate 3749-byte ordinary RET body, excludes compiler/CRT/import/shared-tail ownership. Required engine dependencies retain their separately recorded provenance.',behavior='Complete player event receiver including damage and contact physics, radial impulse, pose/full-state queries, disable counters, damping, money/fines, repair/fine settlement and ten-weapon acquisitions',boundary=dict(start='0041d7c0',end_exclusive='0041e665',bytes=3749,instructions=1110,sha256=hashlib.sha256(body).hexdigest(),ranges=[dict(start='0041d7c0',end_exclusive='0041e665')]),implementation='func',integration='standalone_linked_pending_parent_integration',target='build/player-query/bhunter-player-query-check.exe',validation=dict(abi='x86 cdecl callback entity/packet; source context explicitly binds globals; actual original player offsets and ten real live weapon registry pointers; packet40B; callback installed via real entity query owner; verified RET steps/methods represented by NULL/no-operation.',fixtures=dict(scenario='player-query',checks=124,failures=0,original_code_required=False,play_calls=0),original_code_executed=False,game_executed=False,raw_byte_match=False,limits=['No original differential run; reconstructed functional behavior checked from complete raw static body and bounded source fixtures. PC53 nearest/masked finite moderate physical values; PC24 and exceptional numerical behavior remain unvalidated.','Source contexts, borrowed original-global equivalents, real entity/pool, actor chain, message panel binding, ten live factory records and original pointer table must outlive synchronous callbacks. Complete player construction/tick/startup remain missing.','Pickup packets require indices0..9,20,21; nonnegative quantity/price/money; fine payment requires positive price. Invalid zero-price fine settlement would fault in original and is explicitly rejected before mutation. Signed arithmetic wraps in original32bit fields.','Weapon selection calls actual admitted ready/cancel implementations, including verified RET no-op only. Delayed cue source-private context binding follows existing mission-sound owner at payload400; original104/180 stores and installed whole tick retained.','Packet checksum and actual Send owner linked; focused tests exercise NULL-peer transport only, never a network session/send. No native sound playback or game loop. Errors preserve prior side effects.','Standalone uses exact snapshotted build/m0 admitted dependencies; parent fresh full/clean integration and independent source review are required for admission.']),evidence=['META/m4/target550/a/README.md','META/m4/target550/a/standalone-build.json','META/m4/target550/a/constants.json','META/m4/target550/a/original/0041d7c0.disasm.json'])
(HERE/'owners.json').write_bytes((json.dumps([owner],indent=2)+'\n').encode())
print('Captured complete original owner:',len(body),'bytes,',len(rows),'instructions; zero owned data')
