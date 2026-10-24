"""Audit the source collision closure without executing original game code."""
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    manifest = read(ROOT/'META/reference-manifest.json')
    for row in manifest['files']:
        if row['root']=='repo':
            assert sha(ROOT/row['path'])==row['sha256']
    exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
    pe=struct.unpack_from('<I',exe,60)[0]
    section_table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
    sections=[struct.unpack_from('<IIII',exe,section_table+40*i+8)
              for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
    def image_bytes(address,size):
        rva=address-0x400000
        _,start,length,offset=next(s for s in sections if s[1]<=rva and rva+size<=s[1]+s[2])
        return exe[offset+rva-start:offset+rva-start+size]
    ledger=read(ROOT/'META/function-status.json')
    current=read(ROOT/'build/m0/result.json')
    assert current['passed'] and not current['original_code_required']
    verified=0
    for name in ('collision-traversal','swept-collision'):
        folder=ROOT/'META/m4'/name
        review=read(folder/'review.json')
        validation=read(folder/'validation.json')
        real=read(folder/'integration-real.json')
        clean=read(folder/'clean/clean-checkout.json')['result']
        assert validation['passed'] and real['passed'] and clean['passed']
        assert real['test']==validation['real_total'] and clean['test']==validation['clean_total']
        for source,expected in validation['sources'].items():
            path=ROOT/source
            if sha(path)!=expected:
                path=folder/'sources'/(expected+'-'+path.name+'.txt')
            assert sha(path)==expected==real['sources'][source]==clean['sources'][source],source
        assert real['missing_collision_query_control']==clean['missing_collision_query_control']=='rejected with LNK2001'
        for row in review['records']:
            rows=read(folder/'original'/(row['address']+'.disassembly.json'))
            body=bytearray();at=int(row['address'],16)
            for instruction in rows:
                assert int(instruction['address'],16)==at
                data=bytes.fromhex(instruction['bytes'])
                assert image_bytes(at,len(data))==data
                body.extend(data);at+=len(data)
            assert len(rows)==row['instructions']
            assert hashlib.sha256(body).hexdigest()==row['body_sha256']
            assert at-1==int(row['boundary'].split('-')[1],16)
            record=next(r for r in ledger['records'] if r['id']==row['address']+'-functional-c')
            assert record['implementation']=='func' and record['integration']=='linked_harness' and not record['game_admitted']
            assert record['source_sha256']==sha(ROOT/record['source'])==current['sources'][record['source']]
            verified+=len(rows)
    assert image_bytes(0x489ebc,4)==struct.pack('<f',.5)
    assert image_bytes(0x495e30,12)==b'\0'*12
    assert image_bytes(0x491330,4)==struct.pack('<I',1)
    control=read(ROOT/'META/m4/collision-traversal/alias-regression-control.json')
    assert not control['passed']
    control_output=next(c['output'] for c in control['commands'] if c['label']=='scenario')
    control_scenarios=[json.loads(line) for line in control_output.splitlines() if line.startswith('{')]
    assert next(s for s in control_scenarios if s.get('scenario')=='collision-traversal')['failures']==1
    assert 'rotated source polygon retains all original coordinate components' in control_output
    assert current['missing_sweep_control']=='rejected with LNK2001'
    assert next(s for s in current['scenarios'] if s.get('scenario')=='swept-collision')['checks']==91
    assert next(s for s in current['scenarios'] if s.get('scenario')=='collision-traversal')['checks']==29
    print(json.dumps(dict(passed=True, restored_functions=5, original_instructions_verified=verified,
        real_checks=current['test']['checks'], clean_checks=clean['test']['checks'],
        game_launches=0, original_code_executed=False, source_game_admitted=False)))

if __name__=='__main__':
    main()
