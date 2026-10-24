"""Audit entity dispatch and the explicitly partial physics response closure."""
import hashlib
import json
import struct
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))
def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    for row in read(ROOT/'META/reference-manifest.json')['files']:
        if row['root']=='repo':assert sha(ROOT/row['path'])==row['sha256']
    exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
    pe=struct.unpack_from('<I',exe,60)[0]
    table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
    sections=[struct.unpack_from('<IIII',exe,table+40*i+8)
              for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
    def original(address,size):
        rva=address-0x400000
        _,start,length,offset=next(s for s in sections if s[1]<=rva and rva+size<=s[1]+s[2])
        return exe[offset+rva-start:offset+rva-start+size]
    ledger=read(ROOT/'META/function-status.json');current=read(ROOT/'build/m0/result.json')
    assert current['passed'] and not current['original_code_required']
    instructions=0;completed=0
    for block in ('entity-collision','physics-response'):
        folder=ROOT/'META/m4'/block
        validation=read(folder/'validation.json');review=read(folder/'review.json')
        real=read(folder/'integration-real.json');clean=read(folder/'clean/clean-checkout.json')['result']
        assert validation['passed'] and real['passed'] and clean['passed']
        assert validation['real_total']==real['test'] and validation['clean_total']==clean['test']
        assert not validation['game_launches'] and not validation['original_code_executed']
        for name,expected in validation['sources'].items():
            path=ROOT/name
            if sha(path)!=expected:path=folder/'sources'/(expected+'-'+path.name+'.txt')
            assert sha(path)==expected==real['sources'][name]==clean['sources'][name],name
        for row in review['records']:
            data=bytearray();at=int(row['address'],16)
            rows=read(folder/'original'/(row['address']+'.disassembly.json'))
            for item in rows:
                assert int(item['address'],16)==at
                instruction=bytes.fromhex(item['bytes'])
                assert original(at,len(instruction))==instruction
                data.extend(instruction);at+=len(instruction)
            assert len(rows)==row['instructions']
            assert hashlib.sha256(data).hexdigest()==row['body_sha256']
            assert at-1==int(row['boundary'].split('-')[1],16)
            instructions+=len(rows)
            partial=row['address']=='004124d0'
            identity=row['address']+('-numerical-slice-c' if partial else '-functional-c')
            status=next(r for r in ledger['records'] if r['id']==identity)
            assert status['source_sha256']==sha(ROOT/status['source'])==current['sources'][status['source']]
            assert status['implementation']==('wip' if partial else 'func')
            assert status['integration']=='linked_harness' and not status['game_admitted']
            assert status['validation']['complete_original_function']==(not partial)
            if partial:
                assert len(status['validation']['remaining_dependencies'])>=5
                assert validation['full_004124d0_admitted'] is False
            else:completed+=1
        for constant in review.get('constants',[]):
            raw=bytes.fromhex(constant['hex'])
            assert original(int(constant['address'],16),len(raw))==raw
            assert struct.unpack('<'+constant['type'],raw)[0]==constant['value']
    assert original(0x489260,8)==struct.pack('<d',1.5)
    assert original(0x48c060,12)==struct.pack('<3f',1,0,0)
    assert original(0x48c080,12)==struct.pack('<3f',0,0,1)
    scenarios={s['scenario']:s for s in current['scenarios'] if 'scenario' in s}
    assert scenarios['entity-collision']['checks']==28 and scenarios['entity-collision']['failures']==0
    assert scenarios['physics-response']['checks']==114 and scenarios['physics-response']['failures']==0
    for name in ('entity_collision','physics_response','rotation','sweep','world_query'):
        assert current['missing_'+name+'_control']=='rejected with LNK2001'
    symbols=['_bh_entity_collision_dispatch','_bh_physics_overlap','_bh_mat34_rotate_world_x',
             '_bh_mat34_rotate_world_y','_bh_physics_contact_response']
    for path in [ROOT/'build/m0/bhunter-integration.map',ROOT/'META/m4/physics-response/clean/link-map.txt']:
        link_map=path.read_bytes()
        for symbol in symbols:assert symbol.encode() in link_map
    print(json.dumps(dict(passed=True,complete_functions=completed,physics_owner='004124d0 WIP',
        original_instructions_verified=instructions,real_checks=current['test']['checks'],
        clean_checks=clean['test']['checks'],game_launches=0,original_code_executed=False)))
if __name__=='__main__':main()
