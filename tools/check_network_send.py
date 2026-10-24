"""Audit native unopened-interface transport checks and complete send owners."""
import hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];FOLDER=ROOT/'META/m4/network-send'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    for row in read(ROOT/'META/reference-manifest.json')['files']:
        if row['root']=='repo':assert sha(ROOT/row['path'])==row['sha256']
    exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();pe=struct.unpack_from('<I',exe,60)[0]
    table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
    sections=[struct.unpack_from('<IIII',exe,table+40*i+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
    review=read(FOLDER/'review.json');v=read(FOLDER/'validation.json')
    real=read(FOLDER/'integration-real.json');clean=read(FOLDER/'clean/clean-checkout.json')['result'];current=read(ROOT/'build/m0/result.json')
    ledger=read(ROOT/'META/function-status.json')
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
            data=bytes.fromhex(row['bytes']);rva=at-0x400000
            _,start,length,offset=next(s for s in sections if s[1]<=rva and rva+len(data)<=s[1]+s[2])
            assert exe[offset+rva-start:offset+rva-start+len(data)]==data
            body.extend(data);at+=len(data)
        assert at-1==int(rec['boundary'].split('-')[1],16) and len(rows)==rec['instructions']
        assert hashlib.sha256(body).hexdigest()==rec['body_sha256'];instructions+=len(rows)
        status=next(r for r in ledger['records'] if r['id']==rec['address']+'-functional-c')
        assert status['implementation']=='func' and status['integration']=='linked_harness' and not status['game_admitted']
        assert status['source_sha256']==sha(ROOT/status['source'])==current['sources'][status['source']]
    sdk=read(ROOT/'META/native-directplay-dependencies.json')
    assert v['sdk_manifest_sha256']==sha(ROOT/'META/native-directplay-dependencies.json')
    for build in (real,clean,current):
        assert not build['original_code_required']
        scenario=next(s for s in build['scenarios'] if s.get('scenario')=='network-transport')
        assert scenario['checks']==24 and not scenario['failures'] and not scenario['sessions_opened']
        assert next(s for s in build['scenarios'] if 'native_directplay_create_hresult' in s)['native_directplay_create_hresult']==0
        send=next(s for s in build['scenarios'] if 'native_directplay_unopened_send_hresult' in s)
        assert send['native_directplay_unopened_send_hresult']<0 and not send['successful_sends']
        assert build['missing_network_transport_control']=='rejected with LNK2001'
        native=build['native_directplay_dependencies'];assert native['manifest_sha256']==v['sdk_manifest_sha256']
        for dependency in sdk['files']:assert sha(Path(native['root'])/dependency['path'])==dependency['sha256']
        scoped=[]
        for command in build['commands']:
            if command['label'].startswith('compile-') and ('/I'+str(Path(native['root'])/'include')) in command['argv']:scoped.append(command['label'])
        assert sorted(scoped)==['compile-bh_network_transport','compile-network_transport']
        assert 'ole32.lib' in next(c for c in build['commands'] if c['label']=='link')['argv']
    for name in ('link-map.txt','clean/link-map.txt'):
        data=(FOLDER/name).read_bytes()
        for symbol in (b'_bh_network_send',b'_bh_network_packet_send',b'_bh_packet_finish',b'CoCreateInstance'):assert symbol in data
    source=(ROOT/'src/bh_network_transport.c').read_text()+(ROOT/'tests/network_transport.c').read_text()
    for forbidden in ('IDirectPlay3_InitializeConnection','IDirectPlay3_Open','IDirectPlay3_CreatePlayer'):assert forbidden not in source
    assert not v['successful_delivery_validated'] and not v['game_launches'] and not v['original_code_executed']
    print(json.dumps(dict(passed=True,complete_functions=2,original_instructions_verified=instructions,real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],native_checks=24,successful_sends=0,game_launches=0)))
if __name__=='__main__':main()
