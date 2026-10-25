import csv,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[4];HERE=Path(__file__).resolve().parent
names={
'0042caa0':'bh_menu_network_open','0042cae0':'bh_menu_submenu_first','0042cb10':'bh_menu_submenu_second',
'0042e360':'bh_menu_keys_motion','0042e540':'bh_menu_keys_weapons','0042e690':'bh_menu_keys_camera','0042e7e0':'bh_menu_keys_extra',
'0042ea70':'bh_menu_network_select','0042eae0':'bh_menu_network_address','0042ee50':'bh_menu_network_phone'}
rows={r['address']:r for r in csv.DictReader((ROOT/'analysis/function-map.tsv').open(),delimiter='\t')}
result=[]
for va,name in names.items():
 ins=json.loads((HERE/'original'/f'{va}.disasm.json').read_text(encoding='utf-8-sig'))
 raw=b''.join(bytes.fromhex(x['bytes']) for x in ins)
 assert len(raw)==int(rows[va]['body_bytes']),(va,len(raw),rows[va]['body_bytes'])
 assert len(ins)==int(rows[va]['instructions'])
 result.append(dict(original_va=va,function=name,source='src/bh_menu_settings.c',role='game',
  provenance_confidence='High: UI callback pointers and switch-specific input/profile selection behavior; no CRT/EH/import signature; distinct entry and RET owner boundaries verified',
  boundary=dict(start=va,end_exclusive=f'{int(va,16)+len(raw):08x}',bytes=len(raw),instructions=len(ins),sha256=hashlib.sha256(raw).hexdigest()),
  implementation='func',integration='linked_harness',target='build/target400-menu-settings/bhunter-menu-settings-check.exe',
  validation=dict(original_code_executed=False,game_executed=False,raw_byte_match=False,scenario='menu-settings',
   checks=976449,failures=0,abi='x86 cdecl; explicit borrowed menu/profile/registry binding; source BHInputProfile76 bytes and network record188 bytes',
   limits=['Menu construction and original row label rendering are external owners; supplied row refresh methods retain the verified invocation contract.',
           'Non-NULL refresh integration uses an explicit adapter forwarding to an actual recovered submenu owner; this does not claim recovery of label rendering.',
           'Invalid row/record/text storage rejected; original adjacent-memory string overflow not reproduced.',
           'No game, network session, physical input, audio, original code or gameplay execution.']),
  evidence=['META/m4/target400/a/README.md','META/m4/target400/a/standalone-build.json',f'META/m4/target400/a/original/{va}.decompile.json',f'META/m4/target400/a/original/{va}.disasm.json']))
(HERE/'owners.json').write_text(json.dumps(result,indent=2)+'\n')
