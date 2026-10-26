import hashlib,json,struct,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];data=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
assert hashlib.sha256(data).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',data,0x3c)[0];count=struct.unpack_from('<H',data,pe+6)[0];opts=struct.unpack_from('<H',data,pe+20)[0]
def raw(va,size):
 for i in range(count):
  off=pe+24+opts+i*40;vs,start,rs,rp=struct.unpack_from('<IIII',data,off+8);rva=va-0x400000
  if start<=rva and rva+size<=start+rs:return data[rp+rva-start:rp+rva-start+size]
 raise ValueError(hex(va))
def asm(start,end):return subprocess.check_output(['objdump','-d','-Mintel',f'--start-address=0x{start:x}',f'--stop-address=0x{end:x}','GAME_DATA/BHUNTER.EXE'],cwd=ROOT,text=True)
body=raw(0x403640,45);assert body[-1]==0xc3
(HERE/'00403640.asm.txt').write_text(asm(0x403640,0x40366d));(HERE/'refresh-dependency.asm.txt').write_text(asm(0x4035e0,0x403632));(HERE/'vm-install.asm.txt').write_text(asm(0x4447a0,0x4447f6))
record=dict(original_va='00403640',function='bh_script_flash_tick',source='src/bh_script_flash.c',role='engine_candidate',
 provenance_confidence='Whole45-byte12-instruction ordinary effect lifetime callback installed by VM opcode4; only actual pool-release dependency, distinct RET; not CRT/import/EH/shared tail, historical game/engine authorship unresolved',
 boundary=dict(start='00403640',end_exclusive='0040366d',bytes=45,instructions=12,sha256=hashlib.sha256(body).hexdigest()),implementation='func',integration='standalone_linked_pending_parent_integration',target='build/script-flash/bhunter-script-flash-check.exe',
 validation=dict(abi='Pinned x86 cdecl BHEntityStep, original countdown+e8 and persistent real entity-pool binding, PC53 nearest/masked',fixtures=dict(scenario='script-flash',checks=25,failures=0,play_calls=0),original_code_executed=False,game_executed=False,raw_byte_match=False,
  limits=['Only the whole flash lifetime step is recovered; real light refresh004035e0 requires missing00478540 queue producer and00477ae0 full consumer, so no flash/VM constructor or fake refresh is admitted.',
          'Source helper binds actual pool/countdown/step and retains existing refresh/query; caller must retain entity/pool lifetime and complete visual bindings independently.',
          'PC24/runtime precision transitions and signaling-NaN payload/exception ordering remain open; no game/original code, UI/input, playback or network operation.']),
 evidence=['META/m4/hour407/a2/README.md','META/m4/hour407/a2/00403640.asm.txt','META/m4/hour407/a2/standalone-build.json','META/m4/hour407/a2/vm-install.asm.txt'])
(HERE/'owners.json').write_text(json.dumps([record],indent=2)+'\n')
