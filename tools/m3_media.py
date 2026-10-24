"""Read-only inspection of the pinned MDS/MDF; optional local marker staging.

MDS field meanings cross-checked with libMirage image-mds.h/parser.c (links in
the report). This is a bounded diagnostic for this preserved single-file CD,
not a production disc backend, installer or universal MDS/ISO implementation.
"""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import struct

ROOT=Path(__file__).resolve().parents[1]

def require(condition,message):
    if not condition:raise ValueError(message)
def digest(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda:f.read(1024*1024),b''):h.update(block)
    return h.hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--extract-source',action='store_true',help='Stage real copyright/BIP markers under ignored build/m3/reference-source')
    parser.add_argument('--write-report',action='store_true')
    args=parser.parse_args()
    pinned=json.loads((ROOT/'META/reference-manifest.json').read_text())
    inputs={v['path']:v['sha256'] for v in pinned['files'] if v['path'] in ('BHUNTER.mdf','BHUNTER.mds')}
    for path,sha in inputs.items():require(digest(ROOT/path)==sha,'immutable input hash '+path)
    mds=(ROOT/'BHUNTER.mds').read_bytes();mdf=ROOT/'BHUNTER.mdf';file_size=mdf.stat().st_size
    def span(offset,size):
        require(0<=offset<=len(mds) and 0<=size<=len(mds)-offset,'MDS span');return mds[offset:offset+size]
    def unpack(fmt,offset):return struct.unpack(fmt,span(offset,struct.calcsize(fmt)))
    require(span(0,16)==b'MEDIA DESCRIPTOR' and span(16,2)==b'\x01\x05','pinned MDS dialect')
    require(unpack('<HH',18)==(0,1),'single CD session')
    session_offset=unpack('<I',80)[0]
    start,end,session,blocks,special,first,last,_,track_offset=unpack('<iiHBBHHII',session_offset)
    require(session==1 and blocks==5 and special==3 and (first,last)==(1,2),'pinned session/track count')
    tracks=[]
    with mdf.open('rb') as f:
        for i in range(blocks):
            at=track_offset+80*i;t=span(at,80);point=t[4]
            if point>=0xa0:continue
            require(1<=point<=99 and t[0] in (0xaa,0xa9) and t[1]==0,'supported CD track mode')
            extra=unpack('<I',at+12)[0];sector_size=unpack('<H',at+16)[0];lba=unpack('<I',at+36)[0];offset=unpack('<Q',at+40)[0]
            files,footer=unpack('<II',at+48);pregap,length=unpack('<II',extra)
            require(files==1 and sector_size==2352 and length>0,'single raw-sector file')
            filename_offset,wide,_,_=unpack('<IIII',footer)
            require(wide==1,'UTF16 footer');name=mds[filename_offset:].decode('utf-16le').split('\0',1)[0]
            require(name=='*.mdf','single companion MDF')
            size=length*sector_size;require(offset<=file_size and size<=file_size-offset,'track fits MDF')
            f.seek(offset);h=hashlib.sha256();remaining=size;nonzero=0
            while remaining:
                b=f.read(min(1024*1024,remaining));require(bool(b),'track short read');h.update(b);remaining-=len(b);nonzero+=sum(v!=0 for v in b)
            tracks.append(dict(number=point,mode='MODE1' if t[0]==0xaa else 'AUDIO',descriptor_offset=at,sector_size=sector_size,start_lba=lba,mdf_offset=offset,pregap_sectors=pregap,stored_sectors=length,stored_bytes=size,duration_seconds=length/75,sha256=h.hexdigest(),nonzero_bytes=nonzero,subchannels_stored=False))
        require(len(tracks)==2 and tracks[0]['mdf_offset']==0 and tracks[0]['stored_bytes']==tracks[1]['mdf_offset'] and sum(v['stored_bytes'] for v in tracks)==file_size,'complete contiguous MDF coverage')
        data=tracks[0];require(data['mode']=='MODE1','data first')
        def sectors(lba,size):
            require(0<=lba and 0<=size<=data['stored_bytes'] and lba+(size+2047)//2048<=data['stored_sectors'],'ISO extent within data track')
            out=bytearray()
            for i in range((size+2047)//2048):
                f.seek(data['mdf_offset']+(lba+i)*2352);raw=f.read(2352)
                require(raw[:12]==b'\0'+b'\xff'*10+b'\0' and raw[15]==1,'raw Mode1 sync/mode')
                out+=raw[16:2064]
            return bytes(out[:size])
        descriptors=[];chosen=None
        for lba in range(16,48):
            v=sectors(lba,2048);require(v[1:6]==b'CD001' and v[6]==1,'ISO volume descriptor')
            descriptors.append(dict(lba=lba,type=v[0]))
            if v[0]==2 and v[88:91] in (b'%/@',b'%/C',b'%/E'):chosen=v;volume_lba=lba
            if v[0]==255:break
        require(chosen is not None,'Joliet descriptor')
        def both32(p,at):
            le=struct.unpack_from('<I',p,at)[0];be=struct.unpack_from('>I',p,at+4)[0];require(le==be,'ISO both-endian field');return le
        volume_blocks=both32(chosen,80)
        root=chosen[156:156+chosen[156]];entries=[];visited=set()
        def record(v):
            require(len(v)>=34 and 33+v[32]<=len(v),'directory record bounds')
            return both32(v,2),both32(v,10),v[25],v[33:33+v[32]]
        def walk(parent,extent,size,depth):
            require(depth<=12 and (extent,size) not in visited and len(entries)<4096,'bounded acyclic directory tree')
            visited.add((extent,size));buf=sectors(extent,size);at=0
            while at<len(buf):
                n=buf[at]
                if not n:at=(at//2048+1)*2048;continue
                require(n<=len(buf)-at and at%2048+n<=2048,'directory record extent')
                e,z,flags,name=record(buf[at:at+n]);at+=n
                if name in (b'\0',b'\1'):continue
                name=name.decode('utf-16be').split(';',1)[0]
                require(name not in ('','.','..') and not any(v in name for v in '/\\:'),'safe Joliet filename')
                path=str(PurePosixPath(parent)/name);require(e+(z+2047)//2048<=volume_blocks,'file inside ISO volume')
                if flags&2:walk(path,e,z,depth+1)
                else:
                    require(not flags&128,'multi-extent files unsupported');content=sectors(e,z)
                    entries.append(dict(path=path,extent=e,bytes=z,sha256=hashlib.sha256(content).hexdigest()))
        e,z,flags,_=record(root);require(flags&2,'directory root');walk('',e,z,0)
        markers=[];extract=ROOT/'build/m3/reference-source'
        for item in entries:
            leaf=PurePosixPath(item['path']).name
            if leaf.lower() not in ('copyright.txt','bip_b1.txt','bip_b2.txt'):continue
            content=sectors(item['extent'],item['bytes']);result=dict(**item)
            if leaf.lower()=='copyright.txt':
                checksum=0
                for (word,) in struct.iter_unpack('<I',content[:len(content)//4*4]):checksum^=word
                result.update(checksum_xor_complete_u32=checksum,expected_checksum=0x6b405d08,checksum_matches=checksum==0x6b405d08,ignored_tail_bytes=len(content)%4)
                require(checksum==0x6b405d08,'real copyright checksum')
            if args.extract_source:
                extract.mkdir(parents=True,exist_ok=True);dest=extract/leaf
                if dest.exists():require(dest.read_bytes()==content,'do not overwrite different staged data')
                else:dest.write_bytes(content)
                result['staged_path']=dest.relative_to(ROOT).as_posix()
            markers.append(result)
    for path,sha in inputs.items():require(digest(ROOT/path)==sha,'post-read immutable hash '+path)
    report=dict(date='2026-10-24',passed=True,scope='M3 immutable media inventory/marker recovery only; no launch/playback/scenario validation',inputs=inputs,format_sources=['https://raw.githubusercontent.com/cdemu/cdemu/master/libmirage/images/image-mds/image-mds.h','https://raw.githubusercontent.com/cdemu/cdemu/master/libmirage/images/image-mds/parser.c'],session=dict(number=session,start=start,end=end,first_track=first,last_track=last),tracks=tracks,volume=dict(descriptor_lba=volume_lba,blocks=volume_blocks,block_bytes=2048,joliet=True,descriptors=descriptors),files=entries,source_markers=markers,source_extracted=args.extract_source,m3_complete=False,limits=['Pregaps are descriptor intervals, absent from MDF stored track spans','Audio bytes are preserved/fingerprinted, not yet decoded or playback verified','Only this pinned single-file v1.5 MDS and Mode1/Joliet dialect are handled','No machine registry writes, installer execution, source-binary patches or original-code mapper changes'])
    if args.write_report:
        dest=ROOT/'META/m3';dest.mkdir(exist_ok=True);(dest/'media-inventory.json').write_text(json.dumps(report,indent=2)+'\n',newline='\n')
    print(json.dumps(dict(passed=True,tracks=[{k:v[k] for k in ('number','mode','stored_sectors','duration_seconds')} for v in tracks],files=len(entries),markers=markers,report_written=args.write_report)))

if __name__=='__main__':main()
