import csv
import json
import struct
from pathlib import Path

rows=list(csv.DictReader(Path('analysis/function-map.tsv').open(),delimiter='\t'))
b=Path('GAME_DATA/BHUNTER.EXE').read_bytes()
p=struct.unpack_from('<I',b,60)[0]
o=p+24
sections=[]
for i in range(struct.unpack_from('<H',b,p+6)[0]):
    sh=o+struct.unpack_from('<H',b,p+20)[0]+40*i
    vs,rva,rs,off=struct.unpack_from('<4I',b,sh+8)
    sections.append(dict(name=b[sh:sh+8].split(b'\0')[0].decode(),address=f'{0x400000+rva:08x}',virtual_size=vs,file_offset=off,file_size=rs))
summary=dict(executable_bytes=len(b),sections=sections,mapped_functions=len(rows),verified_crt_functions=sum('library-crt-verified' in r['tags'] for r in rows),candidate_code_pointer_functions=sum('candidate-function-code-pointer' in r['tags'] for r in rows),pre_import_region_functions=sum(int(r['address'],16)<0x47d360 for r in rows),mapped_instruction_bytes=sum(int(r['body_bytes']) for r in rows),unassigned_instruction_bytes=sum(int(r['bytes']) for r in csv.DictReader(Path('analysis/unassigned-code.tsv').open(),delimiter='\t')))
Path('analysis/binary-map.json').write_text(json.dumps(summary,indent=2))
anchors=[
('00401000','Affine-matrix math','engine/reuse candidate','float matrix operations; provenance unknown'),
('00401fd0','Config loader candidate','platform/game support','BHUNTER.CFG paths'),
('004111e0','Script statement parser','game/engine','trigger, detach, sensor, timer, sound commands'),
('004474c0','World initialization candidate','game','level paths and game-specific assets'),
('00449a50','CD audio initialization','platform glue','MCI cdaudio open and mixer enumeration'),
('0044f550','Image loader candidate','engine/reuse candidate','format dispatch and load errors'),
('0045bff0','Renderer initialization','engine/platform','IDirect3D2 and HAL device creation'),
('00471260','DirectInput initialization','platform glue','DirectInputCreateA version 0500'),
('004723c0','WinMain','platform/game glue','window initialization and startup'),
('00473a40','Source check','installation glue','SourceDir and copyright.txt checksum'),
('00475ef0','Network initialization candidate','platform/network','DirectPlay3A and COM CLSID references'),
('00476410','Network initialization candidate','platform/network','DirectPlay3A and COM CLSID references'),
('0047cbd0','VFILE open','engine/reuse candidate','loose/preloaded/archive dispatch; provenance unknown'),
('0047e730','WinMainCRTStartup','verified CRT','SP3 LIBC code-section match with relocations masked'),
]
with Path('analysis/subsystems.tsv').open('w',newline='') as f:
    w=csv.writer(f,delimiter='\t');w.writerow(['address','name','classification','evidence']);w.writerows(anchors)
print(json.dumps(summary,indent=2))
