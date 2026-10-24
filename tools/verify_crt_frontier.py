"""Bounded M1 CRT attribution check: one pinned library, no compiler sweep.

Question: are the untagged allocation/file/FP wrappers actually VC5 SP3 LIBC?
Positive control: already identified __setdefaultprecision. Stop after this library.
"""
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'analysis/astra-review'))
from targeted_probes import parse_coff


def main():
    library=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc/lib/libc.lib')
    raw=library.read_bytes()
    exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
    expected='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    assert hashlib.sha256(exe).hexdigest()==expected
    targets={'_malloc':(0x47dae0,20),'_free':(0x47db90,80),'_fopen':(0x47dc10,21),'_printf':(0x47de50,61),
             '__fpmath':(0x47d660,23),'__cfltcvt_init':(0x47d690,56),
             '__setdefaultprecision':(0x47e930,19)}
    rows=[];pos=8
    while pos+60<=len(raw):
        header=raw[pos:pos+60];size=int(header[48:58].rstrip(b'\0 '))
        obj=raw[pos+60:pos+60+size];pos+=60+size+(size&1)
        if obj[:2]!=b'\x4c\x01':continue
        symbols,sections=parse_coff(obj)
        for symbol in symbols.values():
            name=symbol['name']
            if name not in targets or symbol['section']<=0:continue
            va,length=targets[name];section=sections[symbol['section']]
            assert symbol['value']==0
            code=section['data'];original=exe[va-0x400c00:va-0x400c00+length]
            mask=bytearray(b'\xff'*length);relocations=[]
            for r in section['relocations']:
                offset=r['offset'];assert r['kind'] in (6,20) and offset+4<=length
                mask[offset:offset+4]=b'\0'*4
                operand=struct.unpack_from('<I',original,offset)[0]
                target=(va+offset+4+operand)&0xffffffff if r['kind']==20 else operand
                relocations.append(dict(offset=offset,kind=r['kind'],symbol=r['symbol']['name'],original_target=f'{target:08x}'))
            exact=all((a&m)==(b&m) for a,b,m in zip(code,original,mask)) and len(code)>=length and all(x in (0,0x90,0xcc) for x in code[length:])
            negative=bytearray(original);negative[0]^=1
            assert mask[0]==255 and any((a&m)!=(b&m) for a,b,m in zip(code,negative,mask))
            rows.append(dict(symbol=name,address=f'{va:08x}',body_bytes=length,library_section_bytes=len(code),relocations=relocations,
                             original_hex=original.hex(),library_hex=code.hex(),mask_hex=mask.hex(),relocation_normalized_match=exact,negative_control_rejected=True))
    report=dict(question=__doc__,exe_sha256=expected,library=str(library),library_sha256=hashlib.sha256(raw).hexdigest(),
                comparison='Full observed body plus library alignment; masks are only actual COFF DIR32/REL32 operands; not a raw-byte match',results=rows)
    (ROOT/'META/m1/crt-frontier.json').write_text(json.dumps(report,indent=2)+'\n')
    assert len(rows)==len(targets) and all(r['relocation_normalized_match'] for r in rows),[(r['symbol'],r['relocation_normalized_match']) for r in rows]
    print('Verified seven complete CRT wrapper bodies; positive and negative controls pass')


if __name__=='__main__':main()
