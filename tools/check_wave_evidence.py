"""Independent RIFF data inspection and data-only PCM controls; no game code."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import struct
import zlib

ROOT=Path(__file__).resolve().parents[1]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write-controls',action='store_true')
    args=parser.parse_args()
    with (ROOT/'META/m2/media/readers/boundaries.tsv').open(encoding='utf-8-sig',newline='') as f:
        boundaries={v['address']:v for v in csv.DictReader(f,delimiter='\t')}
    assert boundaries['0047b830']['ranges']=='0047b830-0047b91f' and boundaries['0047b830']['thunk']=='false'
    review=json.loads((ROOT/'META/m2/media/wave-review.json').read_text())
    assert len(review['records'])==11 and len({v['address'] for v in review['records']})==11
    for v in review['records']:
        assert v['body_ranges']==boundaries[v['address']]['ranges'].split('|')
    blob=(ROOT/'GAME_DATA/Bhunter.bin').read_bytes()
    pinned=json.loads((ROOT/'META/reference-manifest.json').read_text())
    assert hashlib.sha256(blob).hexdigest()==next(v['sha256'] for v in pinned['files'] if v['path']=='GAME_DATA/Bhunter.bin')
    controls=[];tags={}
    for item in json.loads((ROOT/'analysis/assets-index.json').read_text()):
        if not item['name'].lower().endswith('.wav'):continue
        data=blob[item['offset']:item['end']]
        assert data[:4]==b'RIFF' and data[8:12]==b'WAVE' and struct.unpack_from('<I',data,4)[0]+8==len(data)
        at=12;fmt=None;pcm=None;layout=[]
        while at<len(data):
            tag,n=struct.unpack_from('<4sI',data,at);end=at+8+n
            assert end+(n&1)<=len(data)
            layout.append(dict(tag=tag.decode('ascii'),offset=at,size=n));tags[tag.decode('ascii')]=tags.get(tag.decode('ascii'),0)+1
            if tag==b'fmt ' and fmt is None:fmt=data[at+8:end]
            if tag==b'data' and pcm is None:pcm=data[at+8:end]
            at=end+(n&1)
        assert fmt is not None and pcm is not None and len(fmt)==16
        tag,ch,rate,byte_rate,align,bits=struct.unpack('<HHIIHH',fmt)
        assert tag==1 and ch==1 and bits in (8,16) and rate in (11025,22050)
        assert align==ch*bits//8 and byte_rate==rate*align and len(pcm)%align==0
        controls.append(dict(**item,sha256=hashlib.sha256(data).hexdigest(),pcm_sha256=hashlib.sha256(pcm).hexdigest(),pcm_crc32=zlib.crc32(pcm)&0xffffffff,sample_bytes=len(pcm),channels=ch,sample_rate=rate,byte_rate=byte_rate,block_align=align,bits=bits,chunks=layout))
    header='/* Independent struct-based RIFF/PCM data controls; no original code. */\n'
    for v in controls:
        header+='{'+json.dumps(v['name'])+','+','.join(str(v[k])+'UL' for k in ('offset','size','sample_bytes','pcm_crc32','sample_rate','byte_rate','channels','block_align','bits'))+'},\n'
    report=dict(date='2026-10-24',archive_sha256=hashlib.sha256(blob).hexdigest(),files=len(controls),sample_bytes=sum(v['sample_bytes'] for v in controls),chunk_counts=tags,assets=controls,header_sha256=hashlib.sha256(header.encode()).hexdigest())
    if args.write_controls:
        (ROOT/'tests/wave_manifest.h').write_text(header,newline='\n')
        (ROOT/'META/m2/media/wave-controls.json').write_text(json.dumps(report,indent=2)+'\n',newline='\n')
    else:
        assert (ROOT/'tests/wave_manifest.h').read_bytes()==header.encode()
        assert json.loads((ROOT/'META/m2/media/wave-controls.json').read_text())==report
    print(json.dumps(dict(passed=True,files=len(controls),sample_bytes=report['sample_bytes'],chunk_counts=tags,controls_written=args.write_controls)))

if __name__=='__main__':main()
