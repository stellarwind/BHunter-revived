"""Audit recovered render-model owners and their linked source variant."""
import csv,hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];FOLDER=ROOT/'META/m4/model-owner'
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
    instructions=0;ledger=read(ROOT/'META/function-status.json')
    for record in read(FOLDER/'review.json')['records']:
        rows=list(csv.DictReader((ROOT/record['original']).open(),delimiter='\t'));at=int(record['address'],16);body=bytearray()
        for row in rows:
            data=bytes.fromhex(row['bytes']);assert int(row['address'],16)==at and original(at,len(data))==data;body.extend(data);at+=len(data)
        assert at-1==int(record['boundary'].split('-')[1],16) and len(rows)==record['instructions']
        assert hashlib.sha256(body).hexdigest()==record['body_sha256'];instructions+=len(rows)
        status=next(r for r in ledger['records'] if r['id']==record['address']+'-functional-c')
        assert status['implementation']=='func' and status['integration']=='linked_harness' and not status['game_admitted']
        assert status['source_sha256']==sha(ROOT/status['source'])
    assert struct.unpack('<12f',original(0x48c030,48))==(1,0,0,0,0,1,0,0,0,0,1,0)
    real=read(FOLDER/'integration-real.json');clean=read(FOLDER/'clean/clean-checkout.json')['result'];v=read(FOLDER/'validation.json')
    assert v['passed'] and v['real_total']==real['test'] and v['clean_total']==clean['test']
    for build in (real,clean):
        assert build['passed'] and not build['original_code_required']
        assert len([c for c in build['commands'] if c['label'].startswith('compile-')])==81
        model=next(s for s in build['scenarios'] if s.get('scenario')=='render-model-owner')
        assert model['checks']==(8039 if build is real else 63) and not model['failures']
        assert sum(k.startswith('missing_') for k in build)==41
        assert build['missing_model_control']=='rejected with LNK2001'
        for name,expected in v['sources'].items():
            if name=='src/bh_scene_snapshot.c':continue
            path=ROOT/name
            if sha(path)!=expected:path=FOLDER/'sources'/(expected+'-'+path.name+'.txt')
            assert sha(path)==expected==build['sources'][name],name
    assets=next(s for s in real['scenarios'] if 'render_models_loaded' in s)
    assert assets==v['assets'] and assets['render_models_loaded']==505 and assets['material_image_bindings']==1346 and assets['null_used_images']==201
    scene=read(FOLDER/'scene-real.json');old=read(ROOT/'META/m4/image-owner/scene-real.json')
    assert scene['passed'] and scene['scene']['triangles']==397 and scene['scene']['textures']==146
    assert scene['scene']['textured_faces']==33011 and scene['image_sha256']==old['image_sha256']==sha(FOLDER/'scene.bmp')
    for name in ('src/bh_model.c','src/bh_scene_snapshot.c'):
        path=ROOT/name
        if sha(path)!=v['sources'][name]:path=FOLDER/'sources'/(v['sources'][name]+'-'+path.name+'.txt')
        assert scene['sources'][name]==sha(path)==v['sources'][name]
    for name in ('link-map.txt','scene-link-map.txt','clean/link-map.txt'):
        data=(FOLDER/name).read_bytes()
        for symbol in (b'_bh_model_load',b'_bh_model_read',b'_bh_texture_load',b'_bh_bsp_read'):assert symbol in data
    assert not v['original_code_executed'] and not v['game_launches'] and v['identical_scene_pixels']
    print(json.dumps(dict(passed=True,complete_functions=2,original_instructions_verified=instructions,models=505,focused_real_checks=8039,focused_clean_checks=63,real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],identical_scene_pixels=True,original_code_executed=False)))
if __name__=='__main__':main()
