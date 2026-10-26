import hashlib,json
from pathlib import Path
HERE=Path(__file__).resolve().parent
owners=[('004025c0','bh_script_particle_create',333,84),('004024c0','bh_script_particle_tick',254,62),('00402470','bh_script_particle_refresh',68,21),('00443390','bh_script_fountain_tick',287,79),('004434b0','bh_script_steam_tick',397,105),('00443640','bh_script_steam_refresh',161,53)]
result=[]
for va,name,size,count in owners:
 ins=json.loads((HERE/'original'/f'{va}.disasm.json').read_text());raw=b''.join(bytes.fromhex(x['bytes']) for x in ins)
 assert len(raw)==size and len(ins)==count,(va,len(raw),len(ins))
 result.append(dict(original_va=va,function=name,source='src/bh_script_particles.c',role='engine_candidate',
  provenance_confidence='Complete independently referenced source effect constructor/step/refresh, actual pool/random/draw calls; not CRT/import/EH/shared tail; game versus proprietary engine authorship unresolved',
  boundary=dict(start=va,end_exclusive=f'{int(va,16)+size:08x}',bytes=size,instructions=count,sha256=hashlib.sha256(raw).hexdigest()),
  implementation='func',integration='linked_harness',target='build/script-particles/bhunter-script-particles-check.exe',
  validation=dict(abi='Pinned x86 cdecl; actual entity pool,84-byte particle state at+e8, real queue methods and source timer actor table; explicit persistent source contexts, no guessed globals/vtables',
    fixtures=dict(scenario='script-particles',checks=446,failures=0,play_calls=0),raw_byte_match=False,original_code_executed=False,
    limits=['Full VM/parser construction and live world resource/actor binding remain separate incomplete owners; emitter binding and context helpers add no owner count.',
            'PC53 nearest/masked exceptions tested; runtime PC24 transitions and NaN payload/signaling exception ordering remain unvalidated.',
            'Texture IDs are explicit borrowed queue identities; no native presentation, gameplay, input/audio/network operation or original code execution.',
            'Caller retains real actor/owner/context/queue/pool lifetime; source membership helpers are not generation-proof after slot recycling.']),
  evidence=['META/m4/hour407/a/README.md','META/m4/hour407/a/standalone-build.json',f'META/m4/hour407/a/original/{va}.decompile.json',f'META/m4/hour407/a/original/{va}.disasm.json']))
(HERE/'owners.json').write_text(json.dumps(result,indent=2)+'\n')
