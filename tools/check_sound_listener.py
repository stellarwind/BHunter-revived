"""Audit listener/configuration restoration and real source/native consumers."""
import hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];FOLDER=ROOT/'META/m4/sound-listener'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
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
    review=read(FOLDER/'review.json');v=read(FOLDER/'validation.json');ledger=read(ROOT/'META/function-status.json')
    real=read(FOLDER/'integration-real.json');clean=read(FOLDER/'clean/clean-checkout.json')['result'];current=read(ROOT/'build/m0/result.json')
    assert v['passed'] and real['passed'] and clean['passed'] and current['passed']
    assert v['real_total']==real['test'] and v['clean_total']==clean['test']
    for name,expected in v['sources'].items():
        p=ROOT/name
        if sha(p)!=expected:p=FOLDER/'sources'/(expected+'-'+p.name+'.txt')
        assert sha(p)==expected==real['sources'][name]==clean['sources'][name]
    instructions=0
    for rec in review['records']:
        at=int(rec['address'],16);body=bytearray();rows=read(FOLDER/'original'/(rec['address']+'.disassembly.json'))
        for row in rows:
            assert int(row['address'],16)==at
            data=bytes.fromhex(row['bytes']);assert original(at,len(data))==data
            body.extend(data);at+=len(data)
        assert at-1==int(rec['boundary'].split('-')[1],16) and len(rows)==rec['instructions']
        assert hashlib.sha256(body).hexdigest()==rec['body_sha256'];instructions+=len(rows)
        status=next(r for r in ledger['records'] if r['id']==rec['address']+'-functional-c')
        assert status['implementation']=='func' and status['integration']=='linked_harness' and not status['game_admitted']
        assert status['source_sha256']==sha(ROOT/status['source'])==current['sources'][status['source']]
    for constant in review['constants']:
        raw=bytes.fromhex(constant['hex']);assert original(int(constant['address'],16),len(raw))==raw
        assert struct.unpack('<'+constant['type'],raw)[0]==constant['value']
    for build in (real,clean,current):
        scenario=next(s for s in build['scenarios'] if s.get('scenario')=='sound-listener')
        assert scenario['checks']==50 and not scenario['failures'] and not scenario['play_calls']
        assert build['missing_sound_listener_control']==build['missing_sound_queue_control']=='rejected with LNK2001'
        assert not build['original_code_required']
    for name in ('link-map.txt','clean/link-map.txt'):
        data=(FOLDER/name).read_bytes()
        for symbol in (b'_bh_sound_listener',b'_bh_sound_set_voice_limit',b'_bh_sound_priority',b'_bh_sound_register_wave',b'_bh_mat34_rotate_y'):assert symbol in data
    for name in ('src/bh_sound_listener.c','src/bh_sound_queue.c','tests/sound_listener.c'):
        text=(ROOT/name).read_text();assert 'IDirectSoundBuffer_Play' not in text and 'SetVolume(' not in text
    assert 'long listener_active;' in (ROOT/'src/bh_sound_queue.h').read_text()
    assert not v['full_playback_0047c200_admitted'] and not v['game_launches'] and not v['original_code_executed']
    assert next(r for r in ledger['records'] if r.get('original_va')=='004124d0')['implementation']=='wip'
    print(json.dumps(dict(passed=True,complete_functions=2,original_instructions_verified=instructions,real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],listener_checks=50,play_calls=0,game_launches=0)))
if __name__=='__main__':main()
