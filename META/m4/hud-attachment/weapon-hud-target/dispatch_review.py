"""Pin the actual ten-weapon callback table; no original code execution."""
from pathlib import Path
import hashlib,json,struct
ROOT=Path(__file__).resolve().parents[4];HERE=Path(__file__).resolve().parent
blob=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
assert hashlib.sha256(blob).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',blob,60)[0];at=pe+24+struct.unpack_from('<H',blob,pe+20)[0]
sections=[struct.unpack_from('<IIII',blob,at+i*40+8) for i in range(struct.unpack_from('<H',blob,pe+6)[0])]
def raw(va,n):
    _,start,_,offset=next(s for s in sections if s[1]<=va-0x400000 and va-0x400000+n<=s[1]+s[2])
    return blob[offset+va-0x400000-start:offset+va-0x400000-start+n]
assert raw(0x47d680,1)==b'\xc3'
rows=[]
for i in range(10):
    callbacks=struct.unpack('<6I',raw(0x48fa60+i*24,24))
    rows.append(dict(index=i,table_address=f'{0x48fa60+i*24:08x}',
        callback_destinations={f'{offset:03x}':f'{address:08x}' for offset,address in zip((0x12c,0x130,0x124,0x128,0x11c,0x120),callbacks)},
        selected_hud_is_verified_ret=callbacks[1]==0x47d680,
        ammo_limit=struct.unpack('<i',raw(0x48fa38+i*4,4))[0]))
result=dict(table_address='0048fa60',bytes=240,raw=raw(0x48fa60,240).hex(),
    sha256=hashlib.sha256(raw(0x48fa60,240)).hexdigest(),records=rows,
    verified_common_ret='0047d680',common_ret_raw='c3',
    factory='00433ea0',factory_bytes=203,factory_raw_sha256=hashlib.sha256(raw(0x433ea0,203)).hexdigest(),
    player_constructor='00421d20',constructor_creates_indices=list(range(10)),default_selected=[0,9],
    original_code_executed=False,source_function_admission_claimed=False,
    limits='Table/constructor review does not admit weapon construction/firing/query/lifetimes. Eight references to the same verified RET are not eight recovered game functions.')
(HERE/'dispatch-table.json').write_text(json.dumps(result,indent=2)+'\n')
label_path=HERE/'original/00427920.label-review.disasm.json'
if label_path.exists():
    label_rows=json.loads(label_path.read_text())[:274];cursor=0x427920
    for row in label_rows:
        b=bytes.fromhex(row['bytes']);assert int(row['address'],16)==cursor and b==raw(cursor,len(b));cursor+=len(b)
    assert cursor==0x427d2c and label_rows[-1]['mnemonic']=='RET'
    label_path.write_text(json.dumps(label_rows,indent=2)+'\n')
    anchors=[row for row in label_rows if row['address'] in ['00427a2e','00427a3a','00427b5f','00427bcb','00427c3f','00427c49','00427c75','00427c81']]
    (HERE/'remote-label-review.json').write_text(json.dumps(dict(owner='00427920',role='game network join packet consumer',
        bytes=1036,instructions=274,whole_raw_body_verified=True,sha256=hashlib.sha256(raw(0x427920,1036)).hexdigest(),
        conclusion='Same remote slot is stored as ushort actor+114 and remote table004dfad8 index; actor+fc=8, name+11e, score+11c. Actor joins004dfb28 marker list. Marker name atlas row and score atlas row share slot identity.',
        anchors=anchors,source_function_admission_claimed=False,original_code_executed=False),indent=2)+'\n')
print('Pinned10realweaponrecords; selectedHUDcallbacks only indices4/5,8references to commonRET excluded from newfunctioncounts')
