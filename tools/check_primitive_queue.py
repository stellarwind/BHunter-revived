"""Audit complete native image draw-buffer owners and controlled fan change."""
import csv,hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];FOLDER=ROOT/'META/m4/primitive-queue'
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
    real=read(FOLDER/'integration-real.json');clean=read(FOLDER/'clean/clean-checkout.json')['result'];v=read(FOLDER/'validation.json')
    assert v['passed'] and v['real_total']==real['test'] and v['clean_total']==clean['test']
    for build in (real,clean):
        assert build['passed'] and not build['original_code_required']
        assert len([c for c in build['commands'] if c['label'].startswith('compile-')])==83
        queue=next(s for s in build['scenarios'] if s.get('scenario')=='texture-draw-queue');assert queue['checks']==61 and not queue['failures']
        assert sum(k.startswith('missing_') for k in build)==42 and build['missing_texture_draw_control']=='rejected with LNK2001'
        for name,expected in v['sources'].items():
            path=ROOT/name
            if sha(path)!=expected:path=FOLDER/'sources'/(expected+'-'+path.name+'.txt')
            assert sha(path)==expected==build['sources'][name],name
    scene=read(FOLDER/'scene-real.json');old=read(ROOT/'META/m4/model-owner/scene-real.json');control=read(FOLDER/'fan-control.json')
    assert scene['passed'] and scene['scene']['triangles']==397 and scene['scene']['textures']==146 and scene['scene']['nonblack_pixels']==old['scene']['nonblack_pixels']==267843
    assert scene['image_sha256']==sha(FOLDER/'scene.bmp')==control['fan_bitmap_sha256']
    assert control['passed'] and control['control_equals_prior'] and control['control_bitmap_sha256']==old['image_sha256']==control['prior_bitmap_sha256']
    assert control['source_draw_sha256']==sha(ROOT/'src/bh_texture_draw.c') and control['control_source_sha256']==sha(FOLDER/'list-control.c.txt')
    assert control['changed_pixels']==1421 and control['changed_channels']==2732 and control['maximum_channel_delta']==41
    prior=(ROOT/'META/m4/model-owner/scene.bmp').read_bytes();fan=(FOLDER/'scene.bmp').read_bytes()
    offset=struct.unpack_from('<I',prior,10)[0];assert prior[:offset]==fan[:offset]
    delta=[(i//3,abs(a-b)) for i,(a,b) in enumerate(zip(prior[offset:],fan[offset:])) if a!=b]
    assert len({p for p,d in delta})==1421 and len(delta)==2732 and max(d for p,d in delta)==41
    for command in control['commands']:assert command['exit_code']==0
    for name in ('src/bh_texture_draw.c','src/bh_scene_view.c'):
        path=ROOT/name
        if sha(path)!=v['sources'][name]:path=FOLDER/'sources'/(v['sources'][name]+'-'+path.name+'.txt')
        assert scene['sources'][name]==sha(path)==v['sources'][name]
    for name in ('link-map.txt','scene-link-map.txt','clean/link-map.txt'):
        data=(FOLDER/name).read_bytes()
        for symbol in (b'_bh_texture_draw_vertex',b'_bh_texture_draw_flush',b'_bh_texture_resident'):assert symbol in data
    assert not v['original_code_executed'] and not v['game_launches'] and not control['original_code_executed']
    print(json.dumps(dict(passed=True,complete_functions=2,original_instructions_verified=instructions,focused_checks=61,real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],native_fan_changed_pixels=1421,triangle_list_control_recovers_prior=True,original_code_executed=False)))
if __name__=='__main__':main()
