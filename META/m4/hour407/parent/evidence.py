"""Verify whole sound startup body and retained native structure contract."""
import sys,json,csv,hashlib,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[4];HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(ROOT/'tools'))
from record_pickup_basic import reference
ledger,raw=reference();address='0047b500';source=ROOT/'META/m4/window-dispatch/next-review'/f'{address}.disasm.json';rows=json.loads(source.read_text());at=int(address,16);body=bytearray()
for row in rows:
    b=bytes.fromhex(row['bytes']);assert int(row['address'],16)==at and raw(at,len(b))==b;body.extend(b);at+=len(b)
inventory=next(r for r in csv.DictReader((ROOT/'analysis/function-map.tsv').open(),delimiter='\t') if r['address']==address)
assert len(body)==int(inventory['body_bytes'])==228 and len(rows)==int(inventory['instructions'])==75 and rows[-1]['mnemonic']=='RET'
shutil.copyfile(source,HERE/f'{address}.disasm.json')
owners=[dict(original_va=address,function='bh_sound_startup',source='src/bh_sound_startup.c',role='engine_candidate',provenance_confidence='Whole native sound initialization owner called by startup; real DirectSound interfaces, native imports only; no CRT/EH/import-thunk/shared-tail admission, historical engine authorship unresolved',boundary=dict(start=address,end_exclusive=f'{at:08x}',bytes=len(body),instructions=len(rows),sha256=hashlib.sha256(body).hexdigest()),validation=dict(abi='Pinned x86 cdecl; actual DirectSound COM methods, original20-byte descriptor prefix and loop flag1; explicit fresh source queue owns actual COM references',limits=['Device Create retries10/100ms, ignored cooperative/Play results and primary-failure success return implemented; only native positive path executed on this host.','Fresh initialized source queue and live application HWND required; occupied/invalid source owner rejected before calls.','Primary native Play is required original behavior; fixture stops immediately, loads no samples and releases both actual interfaces.','Failure-output defensive release and diagnostic fields are source adapters; no forced native API failures or fabricated implementations.','No original/game code, physical input/cursor, UI capture or secondary playback; actual CD audio and full startup remain open.']),evidence=['META/m4/window-dispatch/NEXT.md','META/m4/hour407/parent/README.md','META/m4/hour407/parent/0047b500.disasm.json','META/m4/hour407/parent/standalone-build.json'])]
(HERE/'owners.json').write_bytes((json.dumps(owners,indent=2)+'\n').encode());print('Verified whole0047b500:228bytes/75instructions')
