"""Audit the complete original ring clipper and its current scene binding."""
import csv,hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];FOLDER=ROOT/'META/m4/clipper'
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
    record=read(FOLDER/'review.json')['records'][0];rows=list(csv.DictReader((ROOT/record['original']).open(),delimiter='\t'));at=int(record['address'],16);body=bytearray()
    for row in rows:
        data=bytes.fromhex(row['bytes']);assert int(row['address'],16)==at and original(at,len(data))==data;body.extend(data);at+=len(data)
    assert at-1==int(record['boundary'].split('-')[1],16) and len(rows)==record['instructions']==1511
    assert hashlib.sha256(body).hexdigest()==record['body_sha256']
    assert struct.unpack('<d',original(0x489ee0,8))[0]==1 and struct.unpack('<d',original(0x489f08,8))[0]==-1
    status=next(r for r in read(ROOT/'META/function-status.json')['records'] if r['id']=='00452140-functional-c')
    assert status['implementation']=='func' and status['integration']=='linked_harness' and not status['game_admitted'] and status['source_sha256']==sha(ROOT/status['source'])
    real=read(FOLDER/'integration-real.json');clean=read(FOLDER/'clean/clean-checkout.json')['result'];v=read(FOLDER/'validation.json')
    assert v['passed'] and v['real_total']==real['test'] and v['clean_total']==clean['test']
    for build in (real,clean):
        assert build['passed'] and not build['original_code_required']
        assert len([c for c in build['commands'] if c['label'].startswith('compile-')])==87
        test=next(s for s in build['scenarios'] if s.get('scenario')=='polygon-clipper');assert test['checks']==1139 and not test['failures']
        varied=next(s for s in build['scenarios'] if 'clipper_oracle_cases' in s);assert varied['clipper_oracle_cases']==256 and varied['maximum_scratch_vertices']==10
        assert sum(k.startswith('missing_') for k in build)==44 and build['missing_clipper_control']=='rejected with LNK2001'
        for name,expected in v['sources'].items():
            path=ROOT/name
            if sha(path)!=expected:path=FOLDER/'sources'/(expected+'-'+path.name+'.txt')
            assert sha(path)==expected==build['sources'][name],name
    fixture=read(FOLDER/'six-plane-fixture.json');assert fixture['cuts']==6 and len(fixture['output'])==7
    scene=read(FOLDER/'scene-real.json');assert scene['passed'] and scene['controls']['checks']==28 and scene['texture_controls']['checks']==46
    assert scene['scene']['triangles']==397 and scene['scene']['textures']==146 and scene['scene']['nonblack_pixels']==267844
    for name in ('src/bh_clipper.c','src/bh_scene_view.c','src/bh_scene_view.h'):
        path=ROOT/name
        if sha(path)!=v['sources'][name]:path=FOLDER/'sources'/(v['sources'][name]+'-'+path.name+'.txt')
        assert scene['sources'][name]==sha(path)==v['sources'][name]
    prior=(ROOT/'META/m4/primitive-queue/scene.bmp').read_bytes();current=(FOLDER/'scene.bmp').read_bytes();comparison=read(FOLDER/'scene-comparison.json')
    assert sha(FOLDER/'scene.bmp')==scene['image_sha256']==comparison['new_sha256']
    assert hashlib.sha256(prior).hexdigest()==comparison['prior_sha256']
    offset=struct.unpack_from('<I',prior,10)[0];assert prior[:offset]==current[:offset]
    a=prior[offset:];b=current[offset:]
    assert sum(a[i:i+3]!=b[i:i+3] for i in range(0,len(a),3))==comparison['changed_pixel_colors']==2695
    assert sum((a[i:i+3]!=b'\0\0\0')!=(b[i:i+3]!=b'\0\0\0') for i in range(0,len(a),3))==comparison['changed_coverage_pixels']==3
    for name in ('link-map.txt','scene-link-map.txt','clean/link-map.txt'):
        data=(FOLDER/name).read_bytes()
        for symbol in (b'_bh_clip_polygon',b'_bh_mat34_inverse_general',b'_bh_round_float',b'_bh_texture_draw_flush'):assert symbol in data
    assert not v['original_code_executed'] and not v['game_launches']
    print(json.dumps(dict(passed=True,complete_functions=1,original_instructions_verified=1511,focused_checks=1139,real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],scene_triangles=397,changed_coverage_pixels=3,original_code_executed=False)))
if __name__=='__main__':main()
