"""Audit cached trial contact restoration; executes no game or original code."""
import hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    folder=ROOT/'META/m4/cached-contact'
    for row in read(ROOT/'META/reference-manifest.json')['files']:
        if row['root']=='repo':assert sha(ROOT/row['path'])==row['sha256']
    exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();pe=struct.unpack_from('<I',exe,60)[0]
    table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
    sections=[struct.unpack_from('<IIII',exe,table+40*i+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
    review=read(folder/'review.json')['records'][0];at=int(review['address'],16);body=bytearray()
    for row in read(folder/'original/0044d500.disassembly.json'):
        assert int(row['address'],16)==at
        data=bytes.fromhex(row['bytes']);rva=at-0x400000
        _,start,length,offset=next(s for s in sections if s[1]<=rva and rva+len(data)<=s[1]+s[2])
        assert exe[offset+rva-start:offset+rva-start+len(data)]==data
        body.extend(data);at+=len(data)
    assert len(body)==34 and at==0x44d522 and hashlib.sha256(body).hexdigest()==review['body_sha256']
    v=read(folder/'validation.json');real=read(folder/'integration-real.json');clean=read(folder/'clean/clean-checkout.json')['result'];current=read(ROOT/'build/m0/result.json')
    assert v['passed'] and real['passed'] and clean['passed'] and current['passed']
    assert v['real_total']==real['test'] and v['clean_total']==clean['test']
    for name,expected in v['sources'].items():
        assert sha(ROOT/name)==expected==real['sources'][name]==clean['sources'][name]==current['sources'][name]
    for build in (real,clean):
        scenarios={s['scenario']:s for s in build['scenarios'] if 'scenario' in s}
        for name,count in [('swept-collision',91),('entity-collision',28),('physics-response',114)]:
            assert scenarios[name]['checks']==count and not scenarios[name]['failures']
        assert build['missing_collision_model_control']==build['missing_sweep_control']=='rejected with LNK2001'
        assert not build['original_code_required']
    for p in (folder/'link-map.txt',folder/'clean/link-map.txt'):
        assert b'_bh_collision_cached_contact' in p.read_bytes()
    ledger=read(ROOT/'META/function-status.json');status=next(r for r in ledger['records'] if r['id']=='0044d500-functional-c')
    assert status['source_sha256']==sha(ROOT/status['source']) and status['implementation']=='func'
    assert status['integration']=='linked_harness' and not status['game_admitted']
    assert next(r for r in ledger['records'] if r.get('original_va')=='004124d0')['implementation']=='wip'
    assert 'bh_collision_cached_contact(' in (ROOT/'src/bh_sweep.c').read_text()
    print(json.dumps(dict(passed=True,restored_functions=1,real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],physics_owner='WIP',game_launches=0)))
if __name__=='__main__':main()
