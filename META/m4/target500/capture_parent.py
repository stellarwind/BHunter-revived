import json,hashlib,struct,subprocess
from pathlib import Path
root=Path('.');raw_exe=(root/'GAME_DATA/BHUNTER.EXE').read_bytes();pe=struct.unpack_from('<I',raw_exe,60)[0];n=struct.unpack_from('<H',raw_exe,pe+6)[0];opt=struct.unpack_from('<H',raw_exe,pe+20)[0]
sections=[struct.unpack_from('<IIII',raw_exe,pe+24+opt+i*40+8) for i in range(n)]
def raw(a,n):
 for _,v,l,p in sections:
  r=a-0x400000
  if v<=r and r+n<=v+l:return raw_exe[p+r-v:p+r-v+n]
 raise ValueError(a)
for folder,specs in [('parent',[('00476070',730,'bh_network_address'),('00476360',136,'bh_network_session_callback'),('004763f0',26,'bh_network_session_continue'),('00476410',323,'bh_network_connection'),('00476560',182,'bh_network_sessions_refresh'),('00476620',183,'bh_network_connection_dispose')]),('media',[('00449c20',150,'bh_cd_play_range'),('00449cc0',36,'bh_cd_stop'),('00449d40',88,'bh_cd_set_volume'),('00449df0',88,'bh_wave_set_volume')])]:
 here=root/'META/m4/target500'/folder;items=[]
 for address,size,name in specs:
  a=int(address,16);rows=json.loads((here/(address+'.disasm.json')).read_text());rows=[r for r in rows if a<=int(r['address'],16)<a+size];body=raw(a,size);assert b''.join(bytes.fromhex(r['bytes']) for r in rows)==body,(address,len(body),len(rows));assert rows[-1]['mnemonic']=='RET',address
  (here/(address+'.disasm.json')).write_text(json.dumps(rows,indent=2)+'\n')
  items.append(dict(original_va=address,function=name,source='src/bh_'+('network_sessions' if folder=='parent' else 'media_control')+'.c',role='engine_candidate',provenance_confidence='Complete ordinary native API ownership/callback application behavior; not CRT, import, compiler helper or shared tail. Historical authorship unresolved.',boundary=dict(start=address,end_exclusive=f'{a+size:08x}',bytes=size,instructions=len(rows),sha256=hashlib.sha256(body).hexdigest()),validation=dict(abi='Pinned x86; native SDK interfaces and callback stdcall; source explicit borrowed context replaces original globals.',limits=['Positive physical CD playback and successful endpoint volume writes unexecuted; tests use absent or deliberately invalid devices.' if folder=='media' else 'No host/join/player/session creation or peer gameplay tested. Actual provider initialization success remains native-host dependent; native COM/lobby compound address construction and enumeration error/cleanup plus full callback records tested.','Source-only linked harness; startup and playable game remain incomplete.'])))
 (here/'owners.json').write_text(json.dumps(items,indent=2)+'\n')
 print(folder,len(items))
