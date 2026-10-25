import json,struct,hashlib,pathlib
root=pathlib.Path(__file__).resolve().parents[4]
exe=(root/'GAME_DATA/BHUNTER.EXE').read_bytes()
assert hashlib.sha256(exe).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',exe,0x3c)[0];n=struct.unpack_from('<H',exe,pe+6)[0];opts=struct.unpack_from('<H',exe,pe+20)[0];base=struct.unpack_from('<I',exe,pe+24+28)[0]
sections=[]
for i in range(n):
    at=pe+24+opts+i*40
    vs,va,rs,rp=struct.unpack_from('<IIII',exe,at+8)
    sections.append((va,max(vs,rs),rp))
def raw(va,size):
    rva=va-base
    for start,length,offset in sections:
        if start<=rva and rva+size<=start+length:return exe[offset+rva-start:offset+rva-start+size]
    raise AssertionError(hex(va))
for address,count,size in [('00471e00',155,490),('00479140',47,175)]:
    rows=json.loads((pathlib.Path(__file__).parent/(address+'.raw.json')).read_text(encoding='utf-8-sig'))
    assert len(rows)==count
    content=b''.join(bytes.fromhex(r['bytes']) for r in rows)
    assert len(content)==size and content==raw(int(address,16),size)
    assert rows[-1]['mnemonic']=='RET'
assert raw(0x491980,16)==b'c:\\DUMP%.4d.BMP\0'
assert raw(0x48ff78,3)==b'rb\0'
print('PASS: immutable EXE SHA256; 202 instructions/665 complete body bytes; dump format and rb mode')


