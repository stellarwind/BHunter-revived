import hashlib,json,struct,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];data=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();reference='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
assert hashlib.sha256(data).hexdigest()==reference
pe=struct.unpack_from('<I',data,0x3c)[0];count=struct.unpack_from('<H',data,pe+6)[0];opts=struct.unpack_from('<H',data,pe+20)[0]
def raw(va,size):
 for i in range(count):
  off=pe+24+opts+i*40;_,start,length,pos=struct.unpack_from('<IIII',data,off+8);rva=va-0x400000
  if start<=rva and rva+size<=start+length:return data[pos+rva-start:pos+rva-start+size]
 raise ValueError(hex(va))
owners=[]
for va,name,size,ins in [('004035e0','bh_flash_flare_refresh',82,23),('00478540','bh_flare_queue',167,56),('00477ae0','bh_flare_execute',570,189)]:
 start=int(va,16);body=raw(start,size);listing=json.loads((HERE/'original'/f'{va}.disasm.json').read_text());assert len(listing)==ins and b''.join(bytes.fromhex(x['bytes']) for x in listing)==body
 (HERE/'original'/f'{va}.asm.txt').write_text(subprocess.check_output(['objdump','-d','-Mintel',f'--start-address=0x{start:x}',f'--stop-address=0x{start+size:x}','GAME_DATA/BHUNTER.EXE'],cwd=ROOT,text=True))
 owners.append(dict(original_va=va,function=name,source='src/bh_flash_flare.c',role='engine_candidate',
  provenance_confidence='Whole separately referenced ordinary entity refresh/fixed render queue producer/full flare consumer; native field/math/actual projected-screenquad call chain, no CRT/import/EH/shared tail assignment; historical authorship unresolved',
  boundary=dict(start=va,end_exclusive=f'{start+size:08x}',bytes=size,instructions=ins,sha256=hashlib.sha256(body).hexdigest()),implementation='func',integration='standalone_linked_pending_parent_integration',target='build/flash-flare/bhunter-flash-flare-check.exe',
  validation=dict(abi='Pinned x86 cdecl; explicit actual camera/view/screenquad/texture identity bindings; source semantic84-byte command selector17, mutation+next contract and real pool lifetime, PC53 nearest/masked',
   fixtures=dict(scenario='flash-flare',checks=59,failures=0,play_calls=0),original_code_executed=False,game_executed=False,raw_byte_match=False,
   limits=['Count-zero context/VM state binding and semantic command representation; full VM/parser and game startup flare-image resource loading remain unadmitted.',
           'Positive camera focal/near and integer width>=2/height>0 are explicit source guards; out-of-range table magnitude rejects instead of reading beyond14 rows.',
           'PC53 moderate finite branches and unordered strength exercised; PC24, signaling NaN payload/exception order and extreme CRT sqrt behavior remain unvalidated.',
           'Actual offscreen native white-image additive result and behind-camera blank control; synthetic image is a fixture, not a claim of original flare asset appearance.',
           'Standalone compiles new real TUs and links hash-verified existing real source objects; parent full real/clean build must recompile all integration inputs before admission.',
           'No game/original code, gameplay automation, UI/input capture, audio playback or network operation.']),
  evidence=['META/m4/hour424/a/README.md','META/m4/hour424/a/standalone-build.json','META/m4/hour424/a/flare-table-check.json',f'META/m4/hour424/a/original/{va}.asm.txt']))
(HERE/'owners.json').write_text(json.dumps(owners,indent=2)+'\n')
table=raw(0x4928e8,224);obj=(ROOT/'build/flash-flare/bh_flash_flare.obj').read_bytes();assert table in obj
(HERE/'flare-table-check.json').write_text(json.dumps(dict(passed=True,original_va='004928e8',bytes=224,rows=14,sha256=hashlib.sha256(table).hexdigest(),compiled_actual_source_object_contains_exact_table=True,function_byte_match_claimed=False,rows_decoded=[dict(zip(('position','size','alpha','texture'),struct.unpack_from('<fffI',table,i*16))) for i in range(14)]),indent=2)+'\n')
