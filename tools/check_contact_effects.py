"""Read-only evidence audit for source contact effects and draw command production."""
import hashlib
import json
import struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FOLDER=ROOT/'META/m4/contact-effects'
def read(path):return json.loads(path.read_text(encoding='utf-8-sig'))
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    for row in read(ROOT/'META/reference-manifest.json')['files']:
        if row['root']=='repo':assert sha(ROOT/row['path'])==row['sha256']
    exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();pe=struct.unpack_from('<I',exe,60)[0]
    table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
    sections=[struct.unpack_from('<IIII',exe,table+40*i+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
    def original(address,size):
        rva=address-0x400000
        _,start,length,offset=next(s for s in sections if s[1]<=rva and rva+size<=s[1]+s[2])
        return exe[offset+rva-start:offset+rva-start+size]
    review=read(FOLDER/'review.json');validation=read(FOLDER/'validation.json')
    real=read(FOLDER/'integration-real.json');clean=read(FOLDER/'clean/clean-checkout.json')['result']
    current=read(ROOT/'build/m0/result.json');ledger=read(ROOT/'META/function-status.json')
    assert validation['passed'] and real['passed'] and clean['passed'] and current['passed']
    assert validation['real_total']==real['test'] and validation['clean_total']==clean['test']
    for name,expected in validation['sources'].items():
        path=ROOT/name
        if sha(path)!=expected:path=FOLDER/'sources'/(expected+'-'+path.name+'.txt')
        assert sha(path)==expected==real['sources'][name]==clean['sources'][name],name
    instructions=0
    for record in review['records']:
        rows=read(FOLDER/'original'/(record['address']+'.disassembly.json'))
        at=int(record['address'],16);body=bytearray()
        for row in rows:
            assert int(row['address'],16)==at
            data=bytes.fromhex(row['bytes']);assert original(at,len(data))==data
            body.extend(data);at+=len(data)
        assert at-1==int(record['boundary'].split('-')[1],16)
        assert hashlib.sha256(body).hexdigest()==record['body_sha256']
        assert len(rows)==record['instructions'];instructions+=len(rows)
        status=next(r for r in ledger['records'] if r['id']==record['address']+'-functional-c')
        assert status['implementation']=='func' and status['integration']=='linked_harness' and not status['game_admitted']
        assert status['source_sha256']==current['sources'][status['source']]==sha(ROOT/status['source'])
    for constant in review['constants']:
        raw=bytes.fromhex(constant['hex'])
        assert original(int(constant['address'],16),len(raw))==raw
        assert struct.unpack('<d',raw)[0]==constant['value']
    scenarios={s['scenario']:s for s in real['scenarios']}
    assert scenarios['draw-command-queue']['checks']==57 and not scenarios['draw-command-queue']['failures']
    assert scenarios['contact-effects']['checks']==30 and not scenarios['contact-effects']['failures']
    for source in ('draw_queue','contact_effects'):
        key='missing_'+source+'_control'
        assert real[key]==clean[key]==current[key]=='rejected with LNK2001'
    pending=next(r for r in ledger['records'] if r['original_va']=='004124d0')
    assert pending['implementation']=='wip'
    for symbol in [b'_bh_contact_spark_create',b'_bh_draw_oriented',b'_bh_draw_queue_begin']:
        assert symbol in (FOLDER/'link-map.txt').read_bytes()
        assert symbol in (FOLDER/'clean/link-map.txt').read_bytes()
    print(json.dumps(dict(passed=True,restored_functions=len(review['records']),original_instructions_verified=instructions,
        real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],renderer_backend='unrecovered',
        physics_owner='WIP',game_launches=0,original_code_executed=False)))
if __name__=='__main__':main()
