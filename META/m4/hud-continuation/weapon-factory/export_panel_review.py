"""Read-only, whole-body PE evidence for the source message panel family."""
from pathlib import Path
import csv,hashlib,json,struct,subprocess

ROOT=Path(__file__).resolve().parents[4]
HERE=Path(__file__).resolve().parent
image=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
exe_hash=hashlib.sha256(image).hexdigest()
assert exe_hash=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',image,60)[0]
table=pe+24+struct.unpack_from('<H',image,pe+20)[0]
sections=[struct.unpack_from('<IIII',image,table+i*40+8)
          for i in range(struct.unpack_from('<H',image,pe+6)[0])]
def raw(address,size):
    rva=address-0x400000
    section=next((s for s in sections if s[1]<=rva and rva+size<=s[1]+s[2]),None)
    assert section is not None,(hex(address),size)
    return image[section[3]+rva-section[1]:section[3]+rva-section[1]+size]
base=['ghidra','--projects-dir',(ROOT/'ghidra-projects').as_posix(),
      '--project','Bhunter','--program','BHUNTER.EXE']
mapping={row['address']:row for row in csv.DictReader(
    (ROOT/'analysis/function-map.tsv').open(),delimiter='\t')}
roles={0x00442440:'refresh',0x004428b0:'exit',0x00442950:'typing',
       0x00442b60:'entry',0x00442bf0:'startup',0x00442cd0:'event4_query',
       0x00478b60:'plain_text_producer',
       0x00477e80:'plain_text_consumer'}
records=[]
for address,role in roles.items():
    key=f'{address:08x}';meta=mapping[key]
    rows=json.loads(subprocess.check_output(base+['disasm',f'0x{address:08x}',
                                              '-n',meta['instructions']],text=True))
    body=b''.join(bytes.fromhex(row['bytes']) for row in rows)
    at=address
    for row in rows:
        assert int(row['address'],16)==at
        at+=len(bytes.fromhex(row['bytes']))
    assert len(body)==int(meta['body_bytes']) and body==raw(address,len(body))
    assert rows[-1]['mnemonic']=='RET'
    (HERE/f'{key}.disasm.json').write_text(json.dumps(rows,indent=2)+'\n')
    records.append(dict(address=key,role=role,end_exclusive=f'{at:08x}',
                        bytes=len(body),instructions=len(rows),
                        sha256=hashlib.sha256(body).hexdigest(),
                        direct_calls=[row['operands'][0] for row in rows
                                      if row['mnemonic']=='CALL'],
                        provenance='game panel behavior; historical engine ownership unresolved'))
double_constants={f'{a:08x}':struct.unpack('<d',raw(a,8))[0] for a in
                  (0x00489948,0x004893e8,0x00489db8,0x004894c0,0x00489dc0,
                   0x00489280,0x004899b8,0x00489b78,0x00489558)}
assert double_constants=={'00489948':1.1,'004893e8':1.7,'00489db8':1.05,
                          '004894c0':-0.01,'00489dc0':3.6,'00489280':0.5,
                          '004899b8':-0.6,'00489b78':0.35,'00489558':0.3}
assert raw(0x0048fb9c,4)==b'\xff\xff\xff\xff'
assert raw(0x0048fb98,4)==struct.pack('<I',0x004ce458)
producer=json.loads((HERE/'00478b60.disasm.json').read_text())
consumer=json.loads((HERE/'00477e80.disasm.json').read_text())
assert any(row['address']=='00478ba8' and '0x477e80' in row['operands'][-1]
           for row in producer)
assert [row['operands'][0] for row in consumer if row['mnemonic']=='CALL']==['0x00470900']
out=dict(original_exe_sha256=exe_hash,records=records,
         draw_constants=double_constants,
         original_arena_pointer='004ce458',original_last_index=-1,
         plain_producer_callback_identity='00477e80',
         panel_lifecycle_startup_subset='004474c0',
         startup_subset_only=True,
         supported_ring_icon_domain="'1'..'9' or 'A'..'C'",
         no_original_code_executed=True)
(HERE/'panel-review.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps(dict(records=len(records),bytes=sum(r['bytes'] for r in records),
                      draw_constants=double_constants)))
