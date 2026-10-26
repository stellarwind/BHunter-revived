"""Read-only verification of previously reviewed complete editor input owners."""
import sys,json,hashlib,shutil,csv
from pathlib import Path
ROOT=Path(__file__).resolve().parents[4];sys.path.insert(0,str(ROOT/'tools'))
from record_pickup_basic import reference
HERE=Path(__file__).resolve().parent
ledger,raw=reference();inventory={r['address']:r for r in csv.DictReader((ROOT/'analysis/function-map.tsv').open(),delimiter='\t')}
owners=[]
for address,function in [('00412190','bh_editor_input_setup'),('004100b0','bh_editor_input_tick')]:
    original=ROOT/'META/m4/loading-display/next-review'/(address+'.disasm.json');rows=json.loads(original.read_text());body=bytearray();at=int(address,16)
    for row in rows:
        b=bytes.fromhex(row['bytes']);assert int(row['address'],16)==at and b==raw(at,len(b));body.extend(b);at+=len(b)
    item=inventory[address];assert len(body)==int(item['body_bytes']) and len(rows)==int(item['instructions']) and rows[-1]['mnemonic']=='RET'
    shutil.copyfile(original,HERE/(address+'.disasm.json'))
    owners.append(dict(original_va=address,function=function,source='src/bh_editor_input.c',role='game',provenance_confidence='Reviewed editor control setup and installed editor packet consumer; distinct whole application owners, no CRT/EH/import/shared-tail body',boundary=dict(start=address,end_exclusive=f'{at:08x}',bytes=len(body),instructions=len(rows),sha256=hashlib.sha256(body).hexdigest()),validation=dict(abi='Pinned x86 cdecl; real0x2130 input list, packet+2010, actual entity step+cc; explicit borrowed persistent editor globals',limits=['Editor control family, not player controls or full world construction.','Source NULL/repeated-init/list-failure guards; original assumes list allocation succeeds.','Entity allocation failure preserves constructed list as original; caller releases any dispatcher before list/context destruction.','Valid current list encoded packet must not overlap destination state; actual input packet application ABI is deliberately state,packet in source.','Private entity payload+420 stores context; contexts/list/state/pool must outlive callback; live list rebinding retained.','No game, original code, physical input/cursor, UI capture or playback.']),evidence=['META/m4/loading-display/next-review/WORLD_INPUT_CONTRACT.md','META/m4/resume400/parent/README.md','META/m4/resume400/parent/'+address+'.disasm.json']))
(HERE/'owners.json').write_text(json.dumps(owners,indent=2)+'\n',encoding='utf-8')
print('Verified2 complete editor input owners against immutable PE.')
