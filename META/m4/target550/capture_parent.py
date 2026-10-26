import sys,json,hashlib,re,struct
from pathlib import Path
sys.path.insert(0,'tools');from record_pickup_basic import reference
_,raw=reference();base=Path('META/m4/target550');p=base/'parent'
for folder,source,specs,scenario in [('parent','src/bh_network_game.c',[('004766e0',465,'bh_network_host'),('004768c0',343,'bh_network_join'),('00476a20',527,'bh_network_receive')],'network-game'),('transition','src/bh_menu_transition.c',[('0042cb40',98,'bh_menu_music_fade'),('0042cbb0',779,'bh_menu_music_tick'),('0042cec0',52,'bh_menu_fade_expiry'),('0042cf00',164,'bh_menu_quit_fade'),('0042cfb0',184,'bh_menu_quit_tick'),('0042d070',82,'bh_menu_quit_begin')],'menu-transition')]:
 here=base/folder;items=[];constants={}
 for a,n,name in specs:
  at=int(a,16);r=json.loads((p/(a+'.disasm.json')).read_text());b=raw(at,n);assert b''.join(bytes.fromhex(x['bytes']) for x in r)==b
  items.append(dict(original_va=a,function=name,source=source,role='engine_candidate',provenance_confidence='Complete ordinary native multiplayer/session ownership or registered menu/music callback. Full raw/caller/side-effect review; not CRT/import/compiler or shared-tail code. Historical game/engine authorship unresolved.',boundary=dict(start=a,end_exclusive=f'{at+n:08x}',bytes=n,instructions=len(r),sha256=hashlib.sha256(b).hexdigest()),validation=dict(abi='x86 cdecl with explicit borrowed real source state and native SDK definitions; original globals mapped through shared owning contexts.',limits=['Native session/player success and physical audio/gameplay remain unvalidated. Isolated SDK-declared peer fixture drives positive receive/state branches separately from real native uninitialized-interface calls.' if folder=='parent' else 'PC53 nearest/masked numerical profile; finite quit tick __ftol range. Physical CD availability, positive volume endpoints and actual quit/game loop remain unvalidated. Verified original no-op callback is mapped to disabled NULL source step.']),evidence=['META/m4/target550/'+folder+'/README.md','META/m4/target550/'+folder+'/standalone-build.json','META/m4/target550/parent/'+a+'.disasm.json']))
  for x in r:
   for operand in x['operands']:
    m=re.search(r'(float|double) ptr \[0x(0*489[0-9a-f]+)\]',operand)
    if m:
     addr=int(m[2],16);size=4 if m[1]=='float' else 8;constants[addr]=dict(original_va=f'{addr:08x}',raw=raw(addr,size).hex(),value=struct.unpack('<f' if size==4 else '<d',raw(addr,size))[0])
 if folder=='parent':
  b=raw(0x476c30,275);data=dict(start='00476c30',end_exclusive='00476d43',bytes=275,sha256=hashlib.sha256(b).hexdigest(),raw=b.hex(),independent_function_count=0,note='Five dword targets followed by255byte dispatch map for types3..257. Not executable functions.');(here/'switch-data.json').write_text(json.dumps(data,indent=2)+'\n',newline='\n');items[-1]['owned_data']='META/m4/target550/parent/switch-data.json';constants[0x48a218]=dict(original_va='0048a218',raw=raw(0x48a218,16).hex())
 else:
  constants[0x48f530]=dict(original_va='0048f530',raw=raw(0x48f530,140).split(b'\0')[0].hex());constants[0x47d680]=dict(original_va='0047d680',raw=raw(0x47d680,1).hex(),note='Verified empty RET sentinel, reused via source NULL callback, count0')
 (here/'owners.json').write_text(json.dumps(items,indent=2)+'\n',newline='\n');(here/'constants.json').write_text(json.dumps(list(constants.values()),indent=2)+'\n',newline='\n');print(folder,len(items),len(constants))
print('Source no-op sentinel',raw(0x47d680,1).hex())