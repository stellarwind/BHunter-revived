import hashlib,json
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
instructions=json.loads((HERE/'original/004436f0.disasm.json').read_text())
raw=b''.join(bytes.fromhex(x['bytes']) for x in instructions)
assert len(raw)==270 and len(instructions)==79
record=dict(original_va='004436f0',function='bh_script_attached_sound_tick',source='src/bh_script_attached_sound.c',role='engine_candidate',
 provenance_confidence='Ordinary complete script VM sound-update callback; native entity fields and actual sound calls, not CRT/import/EH/shared tail; game versus proprietary engine attribution unresolved',
 boundary=dict(start='004436f0',end_exclusive='004437fe',bytes=270,instructions=79,sha256=hashlib.sha256(raw).hexdigest()),
 implementation='func',integration='linked_harness',target='build/script-attached-sound/bhunter-script-attached-sound-check.exe',
 validation=dict(abi='Pinned x86 cdecl; verified+f0/f4/fc/100/104/108 fields; persistent actual timer actor table, one-level activity owner and real sound queue; no fabricated entity overlay',
 fixtures=dict(scenario='script-attached-sound',checks=221,failures=0,play_calls=0),original_code_executed=False,game_executed=False,raw_byte_match=False,
 limits=['Full script VM00443ff0 and its allocator004457c0/parser helpers remain incomplete; attached-sound construction here is an explicit count-zero source binding adapter.',
         'Typical VM counter range0..15 and PC53/nearest contract exercised; original runtime PC24 transitions remain open.',
         'Source pool-only streaming sound resources test real request/update/stop state without native playback or real WAV/mission startup binding.',
         'Actor table, context, referenced actor/owner and sound resources must outlive attached entity; no actor lifetime or native resource constructor admission.',
         'No game/original code, native capture/input, gameplay automation or network operation.']),
 evidence=['META/m4/resume400/a/README.md','META/m4/resume400/a/standalone-build.json','META/m4/resume400/a/original/004436f0.decompile.json','META/m4/resume400/a/original/004436f0.disasm.json'])
(HERE/'owners.json').write_text(json.dumps([record],indent=2)+'\n')
