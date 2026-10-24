"""Verify full texture image/cache/palette ownership and its exact source variant."""
import csv,hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];FOLDER=ROOT/'META/m4/image-owner'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    for entry in read(ROOT/'META/reference-manifest.json')['files']:
        if entry['root']=='repo':assert sha(ROOT/entry['path'])==entry['sha256']
    exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();pe=struct.unpack_from('<I',exe,60)[0]
    table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
    sections=[struct.unpack_from('<IIII',exe,table+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
    def original(address,size):
        rva=address-0x400000;_,start,length,offset=next(s for s in sections if s[1]<=rva and rva+size<=s[1]+s[2])
        return exe[offset+rva-start:offset+rva-start+size]
    review=read(FOLDER/'review.json');ledger=read(ROOT/'META/function-status.json');instructions=0
    real=read(FOLDER/'integration-real.json');clean=read(FOLDER/'clean/clean-checkout.json')['result'];v=read(FOLDER/'validation.json')
    for record in review['records']:
        rows=list(csv.DictReader((ROOT/record['original']).open(),delimiter='\t'));at=int(record['address'],16);body=bytearray()
        for row in rows:
            data=bytes.fromhex(row['bytes']);assert int(row['address'],16)==at and original(at,len(data))==data;body.extend(data);at+=len(data)
        assert at-1==int(record['boundary'].split('-')[1],16) and len(rows)==record['instructions']
        assert hashlib.sha256(body).hexdigest()==record['body_sha256'];instructions+=len(rows)
        status=next(r for r in ledger['records'] if r['id']==record['address']+'-functional-c')
        assert status['implementation']=='func' and status['integration']=='linked_harness' and not status['game_admitted']
        assert status['source_sha256']==sha(ROOT/status['source'])
    for address,text in [(0x4917f8,b'\nALLOCERR:\0'),(0x491804,b'\nFORMATERR:\0'),(0x491810,b'\nLOADERR:\0')]:assert original(address,len(text))==text
    for build in (real,clean):
        assert build['passed'] and not build['original_code_required']
        image=next(s for s in build['scenarios'] if s.get('scenario')=='texture-image-owner')
        assert image['checks']==(2409 if build is real else 131) and not image['failures']
        assert build['missing_texture_image_control']==build['missing_texture_tables_control']=='rejected with LNK2001'
        texture=next(s for s in build['scenarios'] if s.get('scenario')=='texture-ownership');assert texture['checks']==46 and not texture['failures']
        for name,expected in v['sources'].items():
            path=ROOT/name
            if sha(path)!=expected:path=FOLDER/'sources'/(expected+'-'+path.name+'.txt')
            assert sha(path)==expected==build['sources'][name],name
    assets=next(s for s in real['scenarios'] if 'image_owner_supported_assets' in s)
    assert assets['image_owner_supported_assets']==454 and assets['catalog_rejected_assets']==5
    assert v['real_total']==real['test'] and v['clean_total']==clean['test']
    for identity in v['recompiled_variants']:
        status=next(r for r in ledger['records'] if r['id']==identity)
        assert status['source_sha256']==sha(ROOT/status['source']) and not status.get('historical_variant')
    scene=read(FOLDER/'scene-real.json');old_scene=read(ROOT/'META/m4/texture-owner/scene-real.json')
    assert scene['passed'] and scene['scene']['textures']==146 and scene['scene']['triangles']==397
    assert scene['image_sha256']==old_scene['image_sha256']==sha(FOLDER/'scene.bmp')
    assert scene['sources']['src/bh_texture_image.c']==sha(ROOT/'src/bh_texture_image.c')
    for name in ('link-map.txt','scene-link-map.txt','clean/link-map.txt'):
        data=(FOLDER/name).read_bytes()
        for symbol in (b'_bh_texture_load',b'_bh_texture_tables_init',b'_bh_texture_palette_tables',b'_bh_pcx_reader_next',b'_bh_texture_resident'):assert symbol in data
    print(json.dumps(dict(passed=True,new_complete_functions=4,original_instructions_verified=instructions,real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],supported_assets=454,catalog_rejections=5,identical_scene_pixels=True,original_code_executed=False)))
if __name__=='__main__':main()
