"""Read-only whole-body and constant evidence for message/empty-ammo family."""
from pathlib import Path
import ast,csv,hashlib,json,re,struct,subprocess
ROOT=Path(__file__).resolve().parents[4];HERE=Path(__file__).resolve().parent
image=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
exe_hash=hashlib.sha256(image).hexdigest()
assert exe_hash=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',image,60)[0]
section_table=pe+24+struct.unpack_from('<H',image,pe+20)[0]
sections=[struct.unpack_from('<IIII',image,section_table+i*40+8)
          for i in range(struct.unpack_from('<H',image,pe+6)[0])]
def raw(va,size):
    rva=va-0x400000
    _,start,_,offset=next(s for s in sections if s[1]<=rva and rva+size<=s[1]+s[2])
    return image[offset+rva-start:offset+rva-start+size]
base=['ghidra','--projects-dir',(ROOT/'ghidra-projects').as_posix(),
      '--project','Bhunter','--program','BHUNTER.EXE']
mapping={r['address']:r for r in csv.DictReader((ROOT/'analysis/function-map.tsv').open(),delimiter='\t')}
records=[]
for address,role in [(0x00442a30,'shared_message_owner'),(0x0042f730,'weapon_index0_1'),
                     (0x004309d0,'weapon_index2'),(0x00432560,'weapon_index4'),
                     (0x00433dc0,'weapon_index9')]:
    key=f'{address:08x}';m=mapping[key]
    rows=json.loads(subprocess.check_output(base+['disasm',f'0x{address:08x}',
                                             '-n',m['instructions']],text=True))
    body=b''.join(bytes.fromhex(row['bytes']) for row in rows)
    at=address
    for row in rows:
        assert int(row['address'],16)==at
        at+=len(bytes.fromhex(row['bytes']))
    assert len(body)==int(m['body_bytes']) and body==raw(address,len(body))
    assert rows[-1]['mnemonic']=='RET'
    (HERE/f'{key}.disasm.json').write_text(json.dumps(rows,indent=2)+'\n')
    records.append(dict(address=key,role=role,end_exclusive=f'{at:08x}',
                        bytes=len(body),instructions=len(rows),
                        sha256=hashlib.sha256(body).hexdigest(),
                        direct_calls=[row['operands'][0] for row in rows if row['mnemonic']=='CALL'],
                        provenance='game behavior; historical engine ownership unresolved'))
message=raw(0x0048f308,512).split(bytes([0]),1)[0]
assert message==b"5Hello Mister,\n\nseems like you're\nfresh out of ammo\non that one!\n\nCome visit one of my\nshops for a refill.\n"
source=(ROOT/'src/bh_weapon_empty.c').read_text()
literal=source.split('const char bh_weapon_empty_warning[]=',1)[1].split(';',1)[0]
source_text=''.join(ast.literal_eval(s) for s in re.findall(r'"(?:\\.|[^"\\])*"',literal))
assert source_text.encode('ascii')==message
assert raw(0x004892c0,4)==bytes.fromhex('00004040')
out=dict(original_exe_sha256=exe_hash,records=records,
         message_address='0048f308',message_utf8=message.decode('ascii'),
         source_warning_literal_matches_pe=True,
         message_sha256=hashlib.sha256(message).hexdigest(),
         message_arena_base='004ce458',message_arena_end_exclusive='004d0458',
         message_ring_base='004d0488',ring_slots=8,ring_slot_bytes=512,
         original_panel_method_identity='00442950',panel_time_clamp_float_bits='40400000',
         original_code_executed=False)
(HERE/'message-review.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps(out))
