"""Independent PCX/RLE and instruction-derived palette data controls."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import random
import struct
import zlib

ROOT=Path(__file__).resolve().parents[1]

def palette_control(raw,mode,color):
    # Integer model of the complete 0044f1c0 branches, not original execution.
    colors=[list(v) for v in struct.iter_unpack('3B',raw)]
    mapping=list(range(256))
    code=lambda c:(c[0]<<16)|(c[1]<<8)|c[2]
    if mode==2:
        colors=[([0,0,0] if max(c)<5 else c) for c in colors]
    key_indices=[i for i in range(1,256) if code(colors[i])==color] if mode not in (0,2) else []
    if key_indices:
        for i in key_indices:mapping[i]=0
    needs_zero=(code(colors[0])!=0) if mode==2 else (mode==0 or code(colors[0])!=color)
    if needs_zero:
        if key_indices:
            mapping[0]=key_indices[-1];colors[key_indices[-1]]=colors[0].copy()
        else:
            mapping[0]=min(range(1,256),key=lambda i:sum(abs(a-b) for a,b in zip(colors[0],colors[i])))
    if mode==2:
        for i in range(1,256):
            if colors[i]==[0,0,0]:mapping[i]=0
    else:
        for i in range(1,256):
            lift=max(0,8-max(colors[i]));colors[i]=[v+lift for v in colors[i]]
    colors[0]=[0,0,0]
    return bytes(v for c in colors for v in c),bytes(mapping)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write-controls',action='store_true')
    args=parser.parse_args()
    with (ROOT/'META/m2/media/readers/boundaries.tsv').open(encoding='utf-8-sig',newline='') as f:
        boundaries={v['address']:v for v in csv.DictReader(f,delimiter='\t')}
    review=json.loads((ROOT/'META/m2/media/pcx-review.json').read_text())
    for v in review['records']:
        assert v['body_ranges']==boundaries[v['address']]['ranges'].split('|') and boundaries[v['address']]['thunk']=='false'
    blob=(ROOT/'GAME_DATA/Bhunter.bin').read_bytes()
    manifest=json.loads((ROOT/'META/reference-manifest.json').read_text())
    assert hashlib.sha256(blob).hexdigest()==next(v['sha256'] for v in manifest['files'] if v['path']=='GAME_DATA/Bhunter.bin')
    assets=[];headers={};total=0
    crc=lambda v:zlib.crc32(v)&0xffffffff
    for item in json.loads((ROOT/'analysis/assets-index.json').read_text()):
        if not item['name'].lower().endswith('.pcx'):continue
        d=blob[item['offset']:item['end']];xmin,ymin,xmax,ymax=struct.unpack_from('<4H',d,4);w=xmax+1;h=ymax+1
        assert d[:4]==bytes([10,5,1,8]) and xmin==ymin==0 and d[65]==1 and struct.unpack_from('<H',d,66)[0]==w and d[-769]==12
        at=128;pixels=bytearray();row_cross=0;zero_runs=0
        while len(pixels)<w*h:
            assert at<len(d)-769
            v=d[at];at+=1;n=1
            if v==192:zero_runs+=1;continue
            if v>192:
                n=v&63;assert at<len(d)-769;v=d[at];at+=1
            if len(pixels)//w!=(len(pixels)+n-1)//w:row_cross+=1
            pixels+=bytes([v])*n
        assert len(pixels)==w*h and at==len(d)-769 and not row_cross and not zero_runs
        raw=d[-768:];key=int.from_bytes(raw[51:54],'big');transforms=[]
        for mode,color in [(0,0),(1,0xff00ff),(2,0),(1,key)]:
            rgb,mapping=palette_control(raw,mode,color)
            transforms.append(dict(mode=mode,color=color,palette_crc32=crc(rgb),indices_crc32=crc(mapping),mapped_pixels_crc32=crc(pixels.translate(mapping))))
        assets.append(dict(**item,width=w,height=h,pixel_bytes=len(pixels),sha256=hashlib.sha256(d).hexdigest(),pixels_sha256=hashlib.sha256(pixels).hexdigest(),pixels_crc32=crc(pixels),palette_crc32=crc(raw),transforms=transforms));total+=len(pixels)
    text='/* Independent PCX bytes and instruction-derived palette controls. */\n'
    for v in assets:
        text+='{'+json.dumps(v['name'])+','+','.join(str(v[k])+'UL' for k in ('offset','size','width','height','pixel_bytes','pixels_crc32','palette_crc32'))+',{' + ','.join('{'+','.join(str(t[k])+'UL' for k in ('mode','color','palette_crc32','indices_crc32','mapped_pixels_crc32'))+'}' for t in v['transforms'])+'}},\n'
    headers['tests/pcx_manifest.h']=text
    # Additional data-only inputs exercise palette branches absent in assets.
    rng=random.Random(0x44f1c0);cases=[];lines=['/* Seeded independent palette fixture bytes and expected CRCs. */']
    for i in range(24):
        raw=bytes(rng.randrange(9) if i%3==0 else rng.randrange(256) for _ in range(768))
        raw=bytearray(raw);key=int.from_bytes(raw[27:30],'big')
        if i%2==0:raw[12:15]=raw[27:30];raw[750:753]=raw[27:30]
        for mode,color in [(0,0),(1,key),(2,0),(-3,0x1000000)]:
            rgb,mapping=palette_control(raw,mode,color);cases.append(dict(input_sha256=hashlib.sha256(raw).hexdigest(),mode=mode,color=color,palette_crc32=crc(rgb),indices_crc32=crc(mapping)))
            lines.append('{{'+','.join(str(v) for v in raw)+'},'+str(mode)+','+str(color)+'UL,'+str(crc(rgb))+'UL,'+str(crc(mapping))+'UL},')
    headers['tests/pcx_palette_cases.h']='\n'.join(lines)+'\n'
    report=dict(date='2026-10-24',source='Independent struct/RLE model and complete integer palette branch model; no original code execution',archive_sha256=hashlib.sha256(blob).hexdigest(),files=len(assets),pixel_bytes=total,assets=assets,palette_cases=cases,headers={p:hashlib.sha256(s.encode()).hexdigest() for p,s in headers.items()})
    if args.write_controls:
        for p,s in headers.items():(ROOT/p).write_text(s,newline='\n')
        (ROOT/'META/m2/media/pcx-controls.json').write_text(json.dumps(report,indent=2)+'\n',newline='\n')
    else:
        for p,s in headers.items():assert (ROOT/p).read_bytes()==s.encode(),p
        assert json.loads((ROOT/'META/m2/media/pcx-controls.json').read_text())==report
    print(json.dumps(dict(passed=True,files=len(assets),pixel_bytes=total,palette_fixture_cases=len(cases),controls_written=args.write_controls)))

if __name__=='__main__':main()
