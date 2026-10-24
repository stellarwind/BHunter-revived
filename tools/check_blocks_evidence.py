"""Independently check pinned BSP payloads and instruction-derived FP controls."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import zlib

ROOT=Path(__file__).resolve().parents[1]
STRIDES=(32,60,16,12,56,8)


def single(value):return struct.unpack('<f',struct.pack('<f',value))[0]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write-controls',action='store_true',help='Explicitly replace generated data controls; retain earlier experiments separately')
    args=parser.parse_args()
    blob=(ROOT/'GAME_DATA/Bhunter.bin').read_bytes()
    pinned=json.loads((ROOT/'META/reference-manifest.json').read_text())
    assert hashlib.sha256(blob).hexdigest()==next(v['sha256'] for v in pinned['files'] if v['path']=='GAME_DATA/Bhunter.bin')
    index=json.loads((ROOT/'analysis/assets-index.json').read_text());by={v['name'].upper():v for v in index}
    manifest=[]
    for item in index:
        if not item['name'].upper().endswith('.BSP'):continue
        data=blob[item['offset']:item['end']];at=0;sections=[];counts=[];bounds=bytearray()
        for stride in STRIDES:
            count=struct.unpack_from('<I',data,at)[0];at+=4
            assert 0<count<=65535 and at+count*stride<=len(data)
            sections.append(data[at:at+count*stride]);counts.append(count);at+=count*stride
        assert at==len(data)
        for row in struct.iter_unpack('<10f4i',sections[4]):
            center=[single(single(row[i]+row[i+3])*0.5) for i in range(3)]
            delta=[row[i+3]-center[i] for i in range(3)]
            # FST + FMUL m32 retains one unrounded x87 operand; y stays PC53.
            squared=single((delta[0]*single(delta[0])+delta[1]*delta[1])+delta[2]*single(delta[2]))
            bounds.extend(struct.pack('<5f',*center,squared,single(math.sqrt(squared))))
        row=struct.unpack_from('<10f4i',sections[4]);axes=[max(abs(row[i]),abs(row[i+3])) for i in range(3)]
        root=struct.unpack('<I',struct.pack('<f',math.sqrt((axes[2]*axes[2]+axes[1]*axes[1])+axes[0]*axes[0])))[0]
        sidecar=by.get(item['name'][:-4].upper()+'.TXT');script=blob[sidecar['offset']:sidecar['end']] if sidecar else b''
        manifest.append(dict(**item,counts=counts,crc32=zlib.crc32(data)&0xffffffff,bounds_crc32=zlib.crc32(bounds)&0xffffffff,root_bits=root,
                             sidecar_present=bool(sidecar),sidecar_bytes=len(script),sidecar_crc32=zlib.crc32(script)&0xffffffff))
    rows=['/* Independent reference-data and instruction-derived bound controls, not original code. */']
    for item in manifest:
        rows.append('{'+json.dumps(item['name'])+','+','.join(str(item[k])+'UL' for k in ['offset','size','crc32'])+',{'+','.join(str(v)+'UL' for v in item['counts'])+'},'+','.join(str(item[k])+'UL' for k in ['bounds_crc32','root_bits'])+','+str(int(item['sidecar_present']))+','+str(item['sidecar_bytes'])+'UL,'+str(item['sidecar_crc32'])+'UL},')
    item=by['BSP\\BLOCKS.TXT'];data=blob[item['offset']:item['end']]
    names=[]
    for line in data.splitlines():
        if not line or line.startswith(b';'):continue
        if line.lower()==b'end':break
        names.append(line.decode('ascii'))
    headers={'tests/bsp_manifest.h':'\n'.join(rows)+'\n','tests/block_names.h':'/* Original list order, data-only capture. */\n'+''.join(json.dumps(n)+',\n' for n in names)}
    report=dict(date='2026-10-24',source='Independent Python struct parsing, binary32 stores and PC53 arithmetic model from 0044b689..0044b6c8; not original-code execution',
        squared_radius_expression='(dx * binary32(dx) + dy * dy) + dz * binary32(dz), then binary32 store; radius reloads it',
        archive_sha256=hashlib.sha256(blob).hexdigest(),file_count=len(manifest),registry_count=len(names),registry_crc32=zlib.crc32(data)&0xffffffff,
        registry_offset=item['offset'],registry_bytes=len(data),manifest=manifest,source_headers={p:hashlib.sha256(s.encode()).hexdigest() for p,s in headers.items()})
    if args.write_controls:
        for path,content in headers.items():(ROOT/path).write_text(content,newline='\n')
        (ROOT/'META/m2/blocks/data-controls.json').write_text(json.dumps(report,indent=2)+'\n',newline='\n')
    else:
        for path,content in headers.items():assert (ROOT/path).read_text()==content,path
        assert json.loads((ROOT/'META/m2/blocks/data-controls.json').read_text())==report
    print(json.dumps(dict(passed=True,bsp_files=len(manifest),registry_models=len(names),bsp_payload_bytes=sum(v['size'] for v in manifest),controls_written=args.write_controls)))


if __name__=='__main__':main()
