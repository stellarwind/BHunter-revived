"""Audit original camera/frustum and viewport source ownership."""
import csv,hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];FOLDER=ROOT/'META/m4/view-camera'
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
    for address,value in [(0x489ef8,.5),(0x489efc,-.5),(0x49175c,1),(0x491760,100000),(0x491764,1.0471975803375244)]:assert struct.unpack('<f',original(address,4))[0]==value
    real=read(FOLDER/'integration-real.json');clean=read(FOLDER/'clean/clean-checkout.json')['result'];v=read(FOLDER/'validation.json')
    assert v['passed'] and v['real_total']==real['test'] and v['clean_total']==clean['test']
    for build in (real,clean):
        assert build['passed'] and not build['original_code_required']
        assert len([c for c in build['commands'] if c['label'].startswith('compile-')])==89
        test=next(s for s in build['scenarios'] if s.get('scenario')=='view-camera');assert test['checks']==1623 and not test['failures']
        assert sum(k.startswith('missing_') for k in build)==45 and build['missing_view_camera_control']=='rejected with LNK2001'
        for name,expected in v['sources'].items():
            if name!='src/bh_scene_snapshot.c':assert sha(ROOT/name)==expected==build['sources'][name],name
        for label in ('compile-bh_view_camera','compile-view_camera'):
            assert not any(a.startswith('/I') for a in next(c for c in build['commands'] if c['label']==label)['argv'])
    scene=read(FOLDER/'scene-real.json');assert scene['passed'] and scene['controls']['checks']==28 and scene['texture_controls']['checks']==46
    assert scene['scene']['triangles']==5881 and scene['scene']['textures']==146 and scene['scene']['nonblack_pixels']==264231
    assert scene['image_sha256']==sha(FOLDER/'scene.bmp')
    for name in ('src/bh_view_camera.c','src/bh_scene_view.c','src/bh_scene_view.h','src/bh_scene_snapshot.c'):assert scene['sources'][name]==sha(ROOT/name)==v['sources'][name]
    for name in ('link-map.txt','scene-link-map.txt','clean/link-map.txt'):
        data=(FOLDER/name).read_bytes()
        for symbol in (b'_bh_view_camera_build',b'_bh_view_reset_viewport',b'_bh_mat34_inverse_rigid',b'_bh_clip_polygon'):assert symbol in data
    assert not v['original_code_executed'] and not v['game_launches']
    print(json.dumps(dict(passed=True,complete_functions=3,original_instructions_verified=instructions,focused_checks=1623,real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],scene_triangles=5881,original_code_executed=False)))
if __name__=='__main__':main()
