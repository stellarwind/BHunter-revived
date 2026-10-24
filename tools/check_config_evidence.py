"""Read-only verification of the scoped M2 review and reference data captures."""
import csv
import hashlib
import json
from pathlib import Path
import struct

ROOT=Path(__file__).resolve().parents[1]


def main():
    review=json.loads((ROOT/'META/m2/config/review.json').read_text())
    binary=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert hashlib.sha256(binary).hexdigest()==review['reference_exe_sha256']
    manifest=json.loads((ROOT/'META/reference-manifest.json').read_text())
    assert review['reference_exe_sha256']==next(r['sha256'] for r in manifest['files'] if r['path']=='GAME_DATA/BHUNTER.EXE')
    pe=struct.unpack_from('<I',binary,60)[0];opt=pe+24
    base=struct.unpack_from('<I',binary,opt+28)[0]
    sections=[]
    for index in range(struct.unpack_from('<H',binary,pe+6)[0]):
        at=opt+struct.unpack_from('<H',binary,pe+20)[0]+40*index
        _,rva,size,raw=struct.unpack_from('<IIII',binary,at+8)
        sections.append((rva,size,raw))
    captures={}
    for row in review['defaults']:
        va=int(row['va'],16)
        start=next(raw+va-base-rva for rva,size,raw in sections if rva<=va-base and va-base+row['bytes']<=rva+size)
        data=binary[start:start+row['bytes']]
        assert data.hex()==row['hex'] and hashlib.sha256(data).hexdigest()==row['sha256']
        captures[va]=data
    settings=captures[0x48f608];profiles=captures[0x48f7a8]
    lines=['/* Immutable-reference data only; no original executable instructions. */','static const unsigned char default_settings[416]={']
    for index in range(0,416,16):lines.append('    '+','.join('0x%02x'%v for v in settings[index:index+16])+',')
    lines+=['};','static const BHInputProfile default_profiles[3]={']
    for index in range(3):
        data=profiles[index*76:(index+1)*76]
        assert data[:12]==bytes(12) and data[72:]==bytes(4)
        lines.append('    {0,NULL,NULL,{'+','.join('0x%02x'%v for v in data[12:72])+'},NULL},')
    lines+=['};','']
    assert (ROOT/'src/bh_config_defaults.h').read_text()=='\n'.join(lines)
    assert captures[0x48ff9c].split(b'\0')[0]==b'BHUNTER.CFG'
    assert bytes(v-1 for v in captures[0x491c68].split(b'\0')[0])==b'BHUNTER.BIN'
    with (ROOT/'META/m2/config/original/boundaries.tsv').open(encoding='utf-8-sig',newline='') as file:
        boundaries={r['address']:r for r in csv.DictReader(file,delimiter='\t')}
    with (ROOT/'analysis/call-edges.tsv').open(encoding='utf-8-sig',newline='') as file:
        calls=list(csv.DictReader(file,delimiter='\t'))
    prior={r['address']:r for r in json.loads((ROOT/'META/m1/role-catalog.json').read_text())['records']}
    assigned={r['address'] for r in review['records']}
    dependencies=[]
    for row in review['records']:
        address=row['address']
        assert row['role'] in ('game','engine_candidate')
        assert boundaries[address]['ranges']==address+'-'+row['end']
        assert boundaries[address]['thunk']=='false'
        assert all((ROOT/p).is_file() for p in row['evidence'])
        for call in (r for r in calls if r['caller']==address):
            target=call['target']
            role='family_member' if target in assigned else 'system_import' if target.startswith('EXTERNAL:') else prior[target]['role']
            assert role in ('family_member','system_import','library_verified','compiler_support'),(address,target,role)
            dependencies.append(dict(source=address,target=target,name=call['name'],role=role))
    print(json.dumps(dict(passed=True,assigned_functions=len(assigned),captured_data_spans=len(captures),dependencies=dependencies),indent=2))


if __name__=='__main__':main()
