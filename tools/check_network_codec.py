"""Audit restored packet codec and reuse of the existing position decoder."""
import hashlib
import json
import struct
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
FOLDER=ROOT/'META/m4/network-codec'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    for row in read(ROOT/'META/reference-manifest.json')['files']:
        if row['root']=='repo':assert sha(ROOT/row['path'])==row['sha256']
    exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
    pe=struct.unpack_from('<I',exe,60)[0]
    table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
    sections=[struct.unpack_from('<IIII',exe,table+40*i+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
    def original(address,size):
        rva=address-0x400000
        _,start,length,offset=next(s for s in sections if s[1]<=rva and rva+size<=s[1]+s[2])
        return exe[offset+rva-start:offset+rva-start+size]
    review=read(FOLDER/'review.json');v=read(FOLDER/'validation.json')
    real=read(FOLDER/'integration-real.json');clean=read(FOLDER/'clean/clean-checkout.json')['result']
    current=read(ROOT/'build/m0/result.json');ledger=read(ROOT/'META/function-status.json')
    assert v['passed'] and real['passed'] and clean['passed'] and current['passed']
    assert v['real_total']==real['test'] and v['clean_total']==clean['test']
    for name,expected in v['sources'].items():
        source=ROOT/name
        if sha(source)!=expected:source=FOLDER/'sources'/(expected+'-'+source.name+'.txt')
        assert sha(source)==expected==real['sources'][name]==clean['sources'][name]
    assert len(review['records'])==4
    instructions=0
    for record in review['records']:
        at=int(record['address'],16);body=bytearray()
        rows=read(FOLDER/'original'/(record['address']+'.disassembly.json'))
        for row in rows:
            assert int(row['address'],16)==at
            data=bytes.fromhex(row['bytes']);assert original(at,len(data))==data
            body.extend(data);at+=len(data)
        assert at-1==int(record['boundary'].split('-')[1],16)
        assert len(rows)==record['instructions']
        assert hashlib.sha256(body).hexdigest()==record['body_sha256']
        instructions+=len(rows)
        status=next(r for r in ledger['records'] if r['id']==record['address']+'-functional-c')
        assert status['implementation']=='func' and status['integration']=='linked_harness' and not status['game_admitted']
        assert status['validation']['complete_original_function']
        assert status['source_sha256']==sha(ROOT/status['source'])==current['sources'][status['source']]
    for constant in review['constants']:
        raw=bytes.fromhex(constant['hex'])
        assert original(int(constant['address'],16),len(raw))==raw
        assert struct.unpack('<'+constant['type'],raw)[0]==constant['value']
    decoder=review['reused_decoder']
    variants=[r for r in ledger['records'] if r.get('original_va')==decoder['address']]
    assert len(variants)==1 and variants[0]['source']==decoder['source']=='src/bh_position.c'
    assert variants[0]['implementation']=='func' and variants[0]['source_sha256']==sha(ROOT/decoder['source'])
    assert 'bh_position_read(&stream,navigation,world,point)' in (ROOT/'src/bh_network_codec.c').read_text()
    for build in (real,clean,current):
        scenario=next(s for s in build['scenarios'] if s.get('scenario')=='network-codec')
        assert scenario['checks']==1599 and scenario['failures']==0 and scenario['network_sends']==0
        assert build['missing_network_codec_control']==build['missing_position_control']=='rejected with LNK2001'
        assert not build['original_code_required']
    for name in ('link-map.txt','clean/link-map.txt'):
        data=(FOLDER/name).read_bytes()
        for symbol in (b'_bh_packet_write_position',b'_bh_packet_write_vector',b'_bh_packet_receive',b'_bh_packet_read_actor',b'_bh_position_read'):
            assert symbol in data
    assert not v['network_sends'] and not v['game_launches'] and not v['original_code_executed']
    assert not v['sender_00423710_admitted'] and not v['transport_00476d50_admitted']
    assert next(r for r in ledger['records'] if r.get('original_va')=='004124d0')['implementation']=='wip'
    print(json.dumps(dict(passed=True,new_functions=4,reused_decoder='00423840',original_instructions_verified=instructions,
        real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],codec_checks=1599,
        network_sends=0,game_launches=0,original_code_executed=False)))
if __name__=='__main__':main()
