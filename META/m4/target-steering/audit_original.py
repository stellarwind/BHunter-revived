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
    bodies=[]
    for name in ('004340f0','004344f0','00410880','004084a0','00434650','00434810','004343c0','00434f40','004355c0','00435790','00435720'):
        source=FOLDER/'original'/(name+'.tsv')
        rows=list(csv.DictReader(source.open(),delimiter='\t'))
        cursor=int(name,16);length=0
        for row in rows:
            address=int(row['address'],16);encoded=bytes.fromhex(row['bytes'])
            assert address==cursor and at(address,len(encoded))==encoded
            cursor+=len(encoded);length+=len(encoded)
        bodies.append(dict(address=name,end_exclusive='%08x'%cursor,
            instruction_count=len(rows),code_bytes=length,
            code_sha256=digest(at(int(name,16),length)),export_sha256=digest(source.read_bytes())))
    assert bodies[0]['code_bytes']==169 and bodies[0]['instruction_count']==53
    assert bodies[1]['code_bytes']==344 and bodies[1]['instruction_count']==116
    rows={row['address']:row for name in ('004340f0','004344f0','00410880','004084a0','00434650','00434810','004343c0','00434f40','004355c0','00435790','00435720')
          for row in csv.DictReader((FOLDER/'original'/(name+'.tsv')).open(),delimiter='\t')}
    anchors={
        '004108a0':'8b4818', # classifier reads portal count
        '004108ab':'895018', # classifier writes incremented portal count
        '0043451d':'8d1c8530724900', # real block registry base
        '00434526':'8b7318', # descriptor portal count
        '0043452f':'81c204020000', # world first portal neighbor
        '00434570':'83c23c', # real60-byte portal stride
        '0043453f':'8b8914010000', # neighbor query_flags
        '004345f8':'69f657e30000', # RNG low product
        '004345fe':'69c0000057e3', # RNG high product
        '0043460b':'8985e8010000', # actor RNG write
        '004088f7':'898db0010000', # constructed minimum.x
        '0040890e':'898dbc010000', # constructed maximum.x
    }
    for address,encoded in anchors.items():assert rows[address]['bytes']==encoded
    scalar_constants={}
    for address,width,fmt in ((0x489280,8,'d'),(0x489288,4,'f'),(0x489c40,8,'d'),(0x489c48,8,'d'),
        (0x489628,4,'f'),(0x4892dc,4,'f'),(0x489360,8,'d'),(0x489448,4,'f'),(0x4894f0,4,'f'),
        (0x489220,8,'d'),(0x4892c8,8,'d'),(0x489218,8,'d'),(0x48930c,4,'f')):
        data=at(address,width);scalar_constants['%08x'%address]=dict(bytes=data.hex(),value=struct.unpack('<'+fmt,data)[0])
    assert at(0x48c070,12).hex()=='000000000000803f00000000'
    scalar_constants['0048c070']=dict(bytes=at(0x48c070,12).hex(),value=[0,1,0])
    list_anchors={'00434f60':'8b86f4000000','00434f66':'8981f4000000',
        '00434f6e':'8b8ef4000000','00434f74':'894840','00434f87':'8990f8000000',
        '00434fcb':'898ef4000000','00434fd1':'89b028010000','00434fdd':'c786f800000000000000',
        '00434feb':'89b0f8000000'}
    # Whole-body verification above covers all node/head writes. Pin additional
    # easy-to-read ownership anchors without inventing an actor link type.
    for address,encoded in list_anchors.items():assert rows[address]['bytes']==encoded
    result=dict(original_sha256=expected,all_exported_bytes_match=True,bodies=bodies,
        verified_contract_anchors=anchors,verified_actor_list_anchors=list_anchors,scalar_constants=scalar_constants,
        ownership='Existing bh_geometry_model_init portal classifier/count and bh_world_geometry_build real portal/bounds owners; registry is runtime-populated, not a static weapon table',
        game_launches=0,original_code_executed=False)
    (FOLDER/'original-audit.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result))
if __name__=='__main__':main()
