"""Audit complete light preparation against immutable bytes and linked evidence."""
import csv,hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];FOLDER=ROOT/'META/m4/light-prepare'
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
    record=read(FOLDER/'review.json')['records'][0]
    rows=list(csv.DictReader((ROOT/record['original']).open(),delimiter='\t'));at=int(record['address'],16);body=bytearray()
    for row in rows:
        data=bytes.fromhex(row['bytes']);assert int(row['address'],16)==at and original(at,len(data))==data;body.extend(data);at+=len(data)
    assert len(rows)==record['instructions']==257 and at==0x450cbb
    assert hashlib.sha256(body).hexdigest()==record['body_sha256']
    for address,kind,value in [(0x489ed0,'f',0),(0x489ed8,'d',.125),(0x489ee0,'d',1),(0x489ee8,'f',1)]:
        assert struct.unpack('<'+kind,original(address,struct.calcsize(kind)))[0]==value
    ledger=read(ROOT/'META/function-status.json');status=next(r for r in ledger['records'] if r['id']=='00450980-functional-c')
    assert status['implementation']=='func' and status['integration']=='linked_harness' and not status['game_admitted']
    assert status['source_sha256']==sha(ROOT/status['source'])
    real=read(FOLDER/'integration-real.json');clean=read(FOLDER/'clean/clean-checkout.json')['result'];v=read(FOLDER/'validation.json')
    assert v['passed'] and v['real_total']==real['test'] and v['clean_total']==clean['test']
    for build in (real,clean):
        assert build['passed'] and not build['original_code_required']
        assert len([c for c in build['commands'] if c['label'].startswith('compile-')])==91
        focus=next(s for s in build['scenarios'] if s.get('scenario')=='light-prepare');assert focus['checks']==991 and not focus['failures']
        assert sum(k.startswith('missing_') for k in build)==46 and build['missing_light_control']=='rejected with LNK2001'
        for name,expected in v['sources'].items():
            path=ROOT/name
            if sha(path)!=expected:path=FOLDER/'sources'/(expected+'-'+path.name+'.txt')
            assert sha(path)==expected,name
            if name!='tools/m0.py':assert expected==build['sources'][name],name
        for label in ('compile-bh_light','compile-light'):
            assert not any(a.startswith('/I') for a in next(c for c in build['commands'] if c['label']==label)['argv'])
    for name in ('link-map.txt','clean/link-map.txt'):
        data=(FOLDER/name).read_bytes()
        for symbol in (b'_bh_light_prepare',b'_bh_mat34_transform_point',b'_bh_mat34_transform_direction',b'_bh_round_float'):assert symbol in data
    assert not v['original_code_executed'] and not v['game_launches']
    print(json.dumps(dict(passed=True,complete_functions=1,original_instructions_verified=257,focused_checks=991,real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],original_code_executed=False)))
if __name__=='__main__':main()
