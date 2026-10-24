"""Read-only source/evidence checks for 0044cc20; never launches the game."""
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FOLDER = ROOT / 'META/m4/face-contact'

def read(p):
    return json.loads(p.read_text(encoding='utf-8-sig'))

def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    review = read(FOLDER / 'review.json')
    exe = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    ledger = read(ROOT / 'META/function-status.json')
    assert hashlib.sha256(exe).hexdigest() == ledger['reference_exe_sha256']
    pe = struct.unpack_from('<I',exe,60)[0]
    table = pe+24+struct.unpack_from('<H',exe,pe+20)[0]
    sections = [struct.unpack_from('<IIII',exe,table+40*i+8)
                for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
    rows = read(FOLDER / 'original/disassembly.json')
    body = bytearray()
    at = 0x44cc20
    for row in rows:
        assert int(row['address'],16) == at
        data = bytes.fromhex(row['bytes'])
        virtual, rva, size, offset = next(s for s in sections if s[1] <= at-0x400000 < s[1]+s[2])
        start = offset+at-0x400000-rva
        assert exe[start:start+len(data)] == data
        body.extend(data)
        at += len(data)
    assert at == 0x44d2ea and len(rows) == 536
    assert hashlib.sha256(body).hexdigest() == review['body_sha256']
    for name, expected in review['original_evidence'].items():
        assert sha(ROOT/name) == expected
    fixture = read(FOLDER/'saved-contact.json')
    assert sha(ROOT/fixture['source']) == fixture['source_sha256']
    assert fixture['event']['fraction_word'] == 0x3f600000
    from check_m3_startup_evidence import events
    captured = next(e for e in events(read(ROOT/fixture['source']))
                    if e['event']=='collision-detector-result' and e['result'])
    assert captured == fixture['event']
    import re
    header = (ROOT/'tests/saved_contact.h').read_text()
    fraction = float(re.search(r'SAVED_CONTACT_FRACTION ([0-9.]+)f',header)[1])
    normal = [float(x) for x in re.findall(r'([-+0-9.e]+)f',header.split('= {')[1])]
    bits = lambda f: struct.unpack('<I',struct.pack('<f',f))[0]
    assert bits(fraction)==captured['fraction_word']
    assert [bits(f) for f in normal]==captured['normal_words']
    real = read(FOLDER/'integration-real.json')
    clean = read(FOLDER/'clean/clean-checkout.json')['result']
    assert real['passed'] and clean['passed']
    validation = read(FOLDER/'validation.json')
    for name, expected in validation['sources'].items():
        source = ROOT/name
        if sha(source) != expected:
            source = FOLDER/'sources'/(expected+'-'+source.name+'.txt')
        assert sha(source) == expected == real['sources'][name] == clean['sources'][name]
    row = next(r for r in ledger['records'] if r['id']=='0044cc20-functional-c')
    assert row['source_sha256'] == sha(ROOT/row['source'])
    assert row['implementation']=='func' and row['integration']=='linked_harness'
    assert real['missing_collision_model_control'] == clean['missing_collision_model_control'] == 'rejected with LNK2001'
    print(json.dumps(dict(passed=True, original_instructions_verified=len(rows),
        real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],
        saved_contact='output-contract regression; original detector inputs unavailable',
        game_launches=0, original_code_executed=False)))

if __name__ == '__main__':
    main()
