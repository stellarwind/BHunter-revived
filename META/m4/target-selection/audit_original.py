"""Read-only byte/boundary/call-site verification; never loads original code."""
import csv
import hashlib
import json
from pathlib import Path
import struct

ROOT=Path(__file__).resolve().parents[3]
FOLDER=Path(__file__).resolve().parent
def digest(data):return hashlib.sha256(data).hexdigest()
def main():
    image=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
    expected='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    assert digest(image)==expected
    pe=struct.unpack_from('<I',image,0x3c)[0]
    sections=struct.unpack_from('<H',image,pe+6)[0]
    optional_size=struct.unpack_from('<H',image,pe+20)[0]
    base=struct.unpack_from('<I',image,pe+24+28)[0]
    section_table=pe+24+optional_size
    mapping=[]
    for i in range(sections):
        entry=section_table+40*i
        size,rva,raw_size,raw=struct.unpack_from('<IIII',image,entry+8)
        mapping.append((base+rva,raw_size,raw,image[entry:entry+8].rstrip(b'\0').decode()))
    def at(address,length):
        for start,size,raw,_ in mapping:
            if start<=address and address+length<=start+size:
                return image[raw+address-start:raw+address-start+length]
        raise ValueError(hex(address))
    source=ROOT/'META/m4/player-frontier/original/00421610.tsv'
    rows=list(csv.DictReader(source.open(),delimiter='\t'))
    cursor=0x421610
    for row in rows:
        address=int(row['address'],16);encoded=bytes.fromhex(row['bytes'])
        assert address==cursor and at(address,len(encoded))==encoded
        cursor+=len(encoded)
    assert cursor==0x421740 and len(rows)==107
    callers=[]
    for start,size,raw,name in mapping:
        if name!='.text':continue
        data=image[raw:raw+size]
        for offset in range(len(data)-4):
            if data[offset]==0xe8 and start+offset+5+struct.unpack_from('<i',data,offset+1)[0]==0x421610:
                callers.append('%08x'%(start+offset))
    result=dict(original_sha256=expected,address='00421610',end_exclusive='00421740',
        instruction_count=len(rows),code_bytes=304,all_exported_bytes_match=True,
        code_sha256=digest(at(0x421610,304)),export_sha256=digest(source.read_bytes()),
        raw_relative_call_candidates=callers,
        limits=['Call-site search is a byte search; caller role evidence comes from retained reviewed frontier.',
                'Current Ghidra project has 744 functions and lacks this function boundary; no database edit made.',
                'No original code or game executed.'])
    (FOLDER/'original-audit.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result))
if __name__=='__main__':main()
