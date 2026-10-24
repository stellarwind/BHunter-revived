"""Audit restored source sound queue and real native non-playing buffer checks."""
import hashlib
import json
import struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];FOLDER=ROOT/'META/m4/sound-queue'
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
    assert real['passed'] and clean['passed'] and current['passed'] and validation['passed']
    assert validation['real_total']==real['test'] and validation['clean_total']==clean['test']
    for name,expected in validation['sources'].items():
        path=ROOT/name
        if sha(path)!=expected:path=FOLDER/'sources'/(expected+'-'+path.name+'.txt')
        assert sha(path)==expected==real['sources'][name]==clean['sources'][name]
    instructions=0
    for record in review['records']:
        rows=read(FOLDER/'original'/(record['address']+'.disassembly.json'));at=int(record['address'],16);body=bytearray()
        for row in rows:
            assert int(row['address'],16)==at
            data=bytes.fromhex(row['bytes']);assert original(at,len(data))==data
            body.extend(data);at+=len(data)
        assert at-1==int(record['boundary'].split('-')[1],16)
        assert hashlib.sha256(body).hexdigest()==record['body_sha256']
        assert len(rows)==record['instructions'];instructions+=len(rows)
        status=next(r for r in ledger['records'] if r['id']==record['address']+'-functional-c')
        assert status['implementation']=='func' and status['integration']=='linked_harness' and not status['game_admitted']
        assert not status['validation']['playback_exercised']
        assert status['source_sha256']==sha(ROOT/status['source'])==current['sources'][status['source']]
    for constant in review['constants']:
        raw=bytes.fromhex(constant['hex']);assert original(int(constant['address'],16),len(raw))==raw
        assert struct.unpack('<d',raw)[0]==constant['value']
    for build in (real,clean):
        scenario=next(s for s in build['scenarios'] if s.get('scenario')=='sound-queue')
        assert scenario['checks']==112 and scenario['failures']==0 and scenario['play_calls']==0
        telemetry=[s for s in build['scenarios'] if 'native_directsound_create_hresult' in s]
        assert len(telemetry)==1 and telemetry[0]['native_directsound_create_hresult']==0
        assert build['missing_sound_queue_control']=='rejected with LNK2001'
        link=next(c for c in build['commands'] if c['label']=='link')
        assert 'dsound.lib' in link['argv'] and 'user32.lib' in link['argv']
    for name in ('src/bh_sound_queue.c','tests/sound_queue.c'):
        source=(ROOT/name).read_text()
        assert 'IDirectSoundBuffer_Play' not in source and '->Play' not in source
        assert 'SetVolume(' not in source
    for path in (FOLDER/'link-map.txt',FOLDER/'clean/link-map.txt'):
        data=path.read_bytes()
        for symbol in [b'_bh_sound_request',b'_bh_sound_stop',b'_bh_sound_priority',b'DirectSoundCreate']:
            assert symbol in data
    pending=next(r for r in ledger['records'] if r['original_va']=='004124d0');assert pending['implementation']=='wip'
    print(json.dumps(dict(passed=True,restored_functions=len(review['records']),original_instructions_verified=instructions,
        real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],native_directsound='real buffers; no Play',
        deferred_playback='unrecovered',game_launches=0,original_code_executed=False)))
if __name__=='__main__':main()
