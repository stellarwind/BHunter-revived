"""Audit the complete deferred sound owner and its source/native checks."""
import hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FOLDER=ROOT/'META/m4/sound-update'
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
    review=read(FOLDER/'review.json');record=review['records'][0];at=int(record['address'],16);body=bytearray()
    rows=read(FOLDER/'original/0047c200.disassembly.json')
    for row in rows:
        assert int(row['address'],16)==at
        data=bytes.fromhex(row['bytes']);assert original(at,len(data))==data
        body.extend(data);at+=len(data)
    assert len(rows)==record['instructions']==666 and at-1==int(record['boundary'].split('-')[1],16)
    assert hashlib.sha256(body).hexdigest()==record['body_sha256']
    for address,fmt,value in [('0048a394','f',0),('0048a3c8','d',1000),('0048a3d0','f',10000),('0048a3d4','f',-10000),('0048a3d8','d',-1/330),('0048a3b0','d',1)]:
        assert struct.unpack('<'+fmt,original(int(address,16),struct.calcsize(fmt)))[0]==value
    v=read(FOLDER/'validation.json');real=read(FOLDER/'integration-real.json');clean=read(FOLDER/'clean/clean-checkout.json')['result']
    assert v['passed'] and real['passed'] and clean['passed']
    assert v['real_total']==real['test'] and v['clean_total']==clean['test']
    for name,expected in v['sources'].items():
        path=ROOT/name
        if sha(path)!=expected:path=FOLDER/'sources'/(expected+'-'+path.name+'.txt')
        assert sha(path)==expected==real['sources'][name]==clean['sources'][name]
    for build in (real,clean):
        scenario=next(s for s in build['scenarios'] if s.get('scenario')=='sound-tick')
        assert scenario['checks']==57 and not scenario['failures'] and scenario['play_calls']==12 and scenario['silent_pcm']
        assert not build['original_code_required'] and build['missing_sound_tick_control']=='rejected with LNK2001'
    status=next(r for r in read(ROOT/'META/function-status.json')['records'] if r['id']=='0047c200-functional-c')
    assert status['implementation']=='func' and status['integration']=='linked_harness' and not status['game_admitted']
    assert status['source_sha256']==sha(ROOT/status['source'])
    for name in ('link-map.txt','clean/link-map.txt'):
        data=(FOLDER/name).read_bytes()
        for symbol in (b'_bh_sound_tick',b'_bh_sound_request',b'_bh_sound_stop',b'_bh_round_float'):assert symbol in data
    assert not v['game_launches'] and not v['original_code_executed']
    print(json.dumps(dict(passed=True,complete_functions=1,original_instructions_verified=len(rows),focused_checks=57,real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],native_silent_play_calls=12,game_launches=0)))
if __name__=='__main__':main()
