"""Audit the general affine inverse prerequisite and linked source variant."""
import csv,hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];FOLDER=ROOT/'META/m4/matrix-inverse'
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
    instructions=0;ledger=read(ROOT/'META/function-status.json')
    for record in read(FOLDER/'review.json')['records']:
        rows=list(csv.DictReader((ROOT/record['original']).open(),delimiter='\t'));at=int(record['address'],16);body=bytearray()
        for row in rows:
            data=bytes.fromhex(row['bytes']);assert int(row['address'],16)==at and original(at,len(data))==data;body.extend(data);at+=len(data)
        assert at-1==int(record['boundary'].split('-')[1],16) and len(rows)==record['instructions']
        assert hashlib.sha256(body).hexdigest()==record['body_sha256'];instructions+=len(rows)
        status=next(r for r in ledger['records'] if r['id']==record['address']+'-functional-c')
        assert status['implementation']=='func' and status['integration']=='linked_harness' and not status['game_admitted']
        assert status['source_sha256']==sha(ROOT/status['source'])
    assert struct.unpack('<f',original(0x4891f0,4))[0]==0 and struct.unpack('<d',original(0x4891e8,8))[0]==1
    real=read(FOLDER/'integration-real.json');clean=read(FOLDER/'clean/clean-checkout.json')['result'];v=read(FOLDER/'validation.json')
    assert v['passed'] and v['real_total']==real['test'] and v['clean_total']==clean['test']
    for build in (real,clean):
        assert build['passed'] and not build['original_code_required']
        assert len([c for c in build['commands'] if c['label'].startswith('compile-')])==85
        test=next(s for s in build['scenarios'] if s.get('scenario')=='matrix-inverse');assert test['checks']==3380 and not test['failures']
        assert sum(k.startswith('missing_') for k in build)==43 and build['missing_matrix_inverse_control']=='rejected with LNK2001'
        for name,expected in v['sources'].items():
            path=ROOT/name
            if sha(path)!=expected:path=FOLDER/'sources'/(expected+'-'+path.name+'.txt')
            assert sha(path)==expected==build['sources'][name],name
        for label in ('compile-bh_matrix_inverse','compile-matrix_inverse'):
            command=next(c for c in build['commands'] if c['label']==label)
            assert not any(a.startswith('/I') for a in command['argv'])
    fixture=read(FOLDER/'store-fixture.json');header=(ROOT/'tests/matrix_inverse_cases.h').read_text()
    for row in fixture['lu']:
        for value in row:assert ('0x%08xUL'%struct.unpack('<I',struct.pack('<f',value))[0]) in header
    for name in ('link-map.txt','clean/link-map.txt'):
        data=(FOLDER/name).read_bytes()
        for symbol in (b'_bh_mat4_lu',b'_bh_mat4_lu_solve',b'_bh_mat34_inverse_general',b'_bh_round_float',b'_bh_mat34_transform_point'):assert symbol in data
    assert not v['original_code_executed'] and not v['game_launches']
    print(json.dumps(dict(passed=True,complete_functions=3,original_instructions_verified=instructions,focused_checks=3380,real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],original_code_executed=False)))
if __name__=='__main__':main()
