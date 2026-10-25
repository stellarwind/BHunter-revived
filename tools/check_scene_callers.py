"""Audit the source caller dependency checkpoint; never launches original code."""
import csv,hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FOLDER=ROOT/'META/m4/scene-callers'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def pixels(path):
    data=path.read_bytes();assert data[:2]==b'BM'
    offset=struct.unpack_from('<I',data,10)[0];width,height=struct.unpack_from('<ii',data,18)
    bits=struct.unpack_from('<H',data,28)[0];assert width>0 and bits in (24,32)
    step=bits//8;stride=(width*step+3)&~3
    return sum(any(data[offset+y*stride+x*step:offset+y*stride+x*step+3]) for y in range(abs(height)) for x in range(width))
def main():
    v=read(FOLDER/'validation.json');real=read(FOLDER/'integration-real.json');cr=read(FOLDER/'clean/clean-checkout.json');clean=cr['result'];owners=read(FOLDER/'owners.json')
    assert v['passed'] and real['passed'] and clean['passed'] and not cr['original_assets_present'] and not cr['compiler_in_checkout']
    assert len(owners)==v['complete_new_functions'] and len({o['address'] for o in owners})==len(owners)
    for name,digest in v['sources'].items():
        assert sha(ROOT/name)==digest,name
        assert sha(ROOT/v['source_snapshots'][name])==digest,name
        if name in real['sources']:assert real['sources'][name]==clean['sources'][name]==digest,name
    for build,prefix in [(real,'real'),(clean,'clean')]:
        assert build['test']==v[prefix+'_total'] and not build['test']['failures'] and not build['original_code_required']
        assert sum(c['label'].startswith('compile-') for c in build['commands'])==v['translation_units']
        controls=[x for k,x in build.items() if k.startswith('missing_')]
        assert len(controls)==v['missing_module_controls'] and all('LNK2001' in x for x in controls)
        for scenario,row in v[prefix+'_focused'].items():
            assert next(r for r in build['scenarios'] if r.get('scenario')==scenario)==row and not row['failures']
        total=sum(r['checks'] for r in build['scenarios'] if 'checks' in r and r.get('scenario')!='m2-foundations')
        assert total==build['test']['checks'],(prefix,total,build['test']['checks'])
    exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();pe=struct.unpack_from('<I',exe,60)[0];table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
    sections=[struct.unpack_from('<IIII',exe,table+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
    def raw(va,n):
        rva=va-0x400000;_,start,_,offset=next(s for s in sections if s[1]<=rva and rva+n<=s[1]+s[2]);return exe[offset+rva-start:offset+rva-start+n]
    ledger=read(ROOT/'META/function-status.json')
    for owner in owners:
        address=owner['address'];at=int(address,16);body=bytearray()
        rows=list(csv.DictReader((ROOT/owner['original']).open(),delimiter='\t'))
        for row in rows:
            assert int(row['address'],16)==at
            b=bytes.fromhex(row['bytes']);body.extend(b);at+=len(b)
        assert len(rows)==owner['instructions'] and len(body)==owner['bytes'] and hashlib.sha256(body).hexdigest()==owner['body_sha256']
        assert body==raw(int(address,16),len(body))
        record=next(r for r in ledger['records'] if r['id']==address+'-functional-c')
        assert record['implementation']=='func' and record['integration']=='linked_harness' and not record['game_admitted']
        assert record['source_sha256']==sha(ROOT/record['source']) and not record['validation']['raw_byte_match']
        for path in ['link-map.txt','clean/link-map.txt']:
            assert ('_'+record['function']).encode() in (FOLDER/path).read_bytes(),record['function']
    ct=read(FOLDER/'camera-table.json');assert raw(int(ct['address'],16),48)==bytes.fromhex(ct['bytes'])
    for a in v['unimplemented_callers']:
        records=[r for r in ledger['records'] if r.get('original_va')==a]
        assert records and all(r['implementation'] in ['not_started','wip'] for r in records)
    for name,digest in v['native_images'].items():
        path=FOLDER/name;assert sha(path)==digest
        assert (pixels(path)==0) if 'blank' in name else (pixels(path)>0),name
    for name in ['scene-callers-native-cached.bmp','scene-callers-native-after-gate.bmp']:
        assert v['native_images'][name]==v['native_images']['scene-callers-native-first.bmp']
    assert v['native_images']['scene-callers-native-second.bmp']!=v['native_images']['scene-callers-native-first.bmp']
    for item in read(ROOT/'META/reference-manifest.json')['files']:
        if item['root']=='repo':assert sha(ROOT/item['path'])==item['sha256'],item['path']
    assert not v['original_code_executed'] and not v['game_launches'] and not v['gameplay_executed']
    print(json.dumps(dict(passed=True,complete_new_functions=len(owners),translation_units=v['translation_units'],real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],missing_module_controls=v['missing_module_controls'],game_launches=0)))
if __name__=='__main__':main()
