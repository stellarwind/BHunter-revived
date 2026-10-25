"""Read-only PCX close ownership reconciliation and image contract anchors."""
import hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[4];HERE=Path(__file__).resolve().parent
exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();sha=hashlib.sha256(exe).hexdigest()
assert sha=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',exe,60)[0];t=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
sections=[struct.unpack_from('<IIII',exe,t+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
def raw(va,n):
 rva=va-0x400000;_,r,_,o=next(s for s in sections if s[1]<=rva and rva+n<=s[1]+s[2]);return exe[o+rva-r:o+rva-r+n]
ledger=json.loads((ROOT/'META/function-status.json').read_text())['records']
rows=json.loads((ROOT/'META/m4/sound-loader/next-review/0044f1b0.disasm.json').read_text())
at=0x44f1b0;body=bytearray()
for row in rows:
 b=bytes.fromhex(row['bytes']);assert int(row['address'],16)==at and b==raw(at,len(b));at+=len(b);body+=b
assert len(body)==15 and len(rows)==5 and at==0x44f1bf and rows[-1]['mnemonic']=='RET'
source=ROOT/'src/bh_pcx_reader.c';source_sha=hashlib.sha256(source.read_bytes()).hexdigest()
related=[r for r in ledger if r.get('original_va') in ['0044f0a0','0044efa0','0044f1c0'] or r['id']=='bh_pcx_reader_open-source-lifecycle']
report=dict(reference_exe_sha256=sha,address='0044f1b0',role='engine_candidate',classification='complete ordinary owning PCX-stream close wrapper; not CRT/import/compiler support/shared tail; historical provenance unresolved',body_bytes=15,instructions=5,body_sha256=hashlib.sha256(body).hexdigest(),raw_instructions=rows,callee=dict(address='0047ce50',function='bh_vfile_close',already_admitted=True),caller=dict(address='00449e50',call='00449f36',palette_lut_branch_after_close='00449f3b'),source='src/bh_pcx_reader.c',source_sha256=source_sha,function='bh_pcx_reader_close',implementation='adapter',original_function_count=0,historical_adapter_preserved='bh_pcx_reader_open-source-lifecycle',new_source_variant=False,production_source_changed=False,original_behavior='closes current owning VFILE only, retaining palette/map/global dimensions',source_extension='NULL/empty/repeated-close guard; return status; zero complete scoped reader including palette/map/dimensions; error reporting',required_seam='loading owner must retain transformed palette and index map before source close; original builds LUT after close',validation='standalone-build.json: actual valid/missing/private stream ownership, exclusive-open positive control before/after close, retained caller palette, empty/repeated/reopen state extensions',existing_records=[dict(id=r['id'],implementation=r['implementation'],function=r['function']) for r in related],current_source_matches_admitted=[dict(id=r['id'],matches=r.get('source_sha256')==source_sha) for r in related if r.get('source')=='src/bh_pcx_reader.c' and r.get('source_sha256')])
(HERE/'close-reconciliation.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(dict(body_bytes=15,instructions=5,original_function_count=0,source_sha256=source_sha,unchanged=all(r['matches'] for r in report['current_source_matches_admitted']))))
