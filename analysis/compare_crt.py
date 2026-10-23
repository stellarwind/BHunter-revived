"""Compare COFF library code sections, masking relocation operands, with the EXE."""
import json
import struct
from pathlib import Path

exe = Path('GAME_DATA/BHUNTER.EXE').read_bytes()
results = []
pe = struct.unpack_from('<I', exe, 60)[0]
section_table = pe + 24 + struct.unpack_from('<H', exe, pe+20)[0]
pe_sections = []
for i in range(struct.unpack_from('<H', exe, pe+6)[0]):
    sh = section_table + 40*i
    virtual_size, rva, raw_size, raw_offset = struct.unpack_from('<IIII', exe, sh+8)
    pe_sections.append((raw_offset, raw_size, 0x400000+rva))
def va(offset):
    return next(base+offset-raw for raw,size,base in pe_sections if raw<=offset<raw+size)
for libpath in [
    Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc/lib/libc.lib'),
    Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc/lib/libcmt.lib'),
    Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC/LIB/LIBC.LIB'),
    Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC/LIB/LIBCMT.LIB'),
    Path('D:/Decomp/MSCVC/MSVC41/VC41/MSDEV/LIB/LIBC.LIB'),
    Path('D:/Decomp/MSCVC/MSVC41/VC41/MSDEV/LIB/LIBCMT.LIB'),
    *[Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC/LIB')/name for name in ('LIBCP.LIB','LIBCPMT.LIB','LIBCI.LIB','GLAUX.LIB','GLU32.LIB','D3DRM.LIB','DPLAY.LIB','DDRAW.LIB','DSOUND.LIB','UUID.LIB','LARGEINT.LIB','OLDNAMES.LIB')],
]:
    lib = libpath.read_bytes()
    assert lib[:8] == b'!<arch>\n'
    pos = 8
    longnames = b''
    hits = []
    eligible = 0
    while pos + 60 <= len(lib):
        header = lib[pos:pos+60]
        if header[58:60] != b'`\n':
            raise ValueError(f'Malformed archive header in {libpath} at {pos:#x}: {header!r}')
        size = int(header[48:58].rstrip(b'\0 '))
        obj = lib[pos+60:pos+60+size]
        pos += 60 + size + (size & 1)
        member = header[:16].decode().strip()
        if member == '//':
            longnames = obj
        elif member.startswith('/') and member[1:].isdigit():
            start = int(member[1:])
            member = longnames[start:].split(b'\0')[0].split(b'\n')[0].decode(errors='replace').rstrip('/')
        if len(obj) < 20 or obj[:2] != b'\x4c\x01':
            continue
        count = struct.unpack_from('<H', obj, 2)[0]
        optional = struct.unpack_from('<H', obj, 16)[0]
        symptr, nsym = struct.unpack_from('<II', obj, 8)
        strtab = symptr+nsym*18
        symbols = []
        si = 0
        while symptr and si < nsym:
            so = symptr+si*18
            rawname = obj[so:so+8]
            if rawname[:4] == b'\0'*4:
                st = strtab+struct.unpack_from('<I',rawname,4)[0]
                symbol = obj[st:].split(b'\0')[0].decode(errors='replace')
            else:
                symbol = rawname.split(b'\0')[0].decode(errors='replace')
            value, sec, typ, storage, aux = struct.unpack_from('<IhHBB',obj,so+8)
            if sec > 0 and storage in (2,3) and (typ & 0x20):
                symbols.append((sec,value,symbol))
            si += 1+aux
        for i in range(count):
            sh = 20 + optional + 40*i
            if sh+40 > len(obj):
                break
            name = obj[sh:sh+8].split(b'\0')[0]
            if not name.startswith(b'.text'):
                continue
            length, raw, relocs = struct.unpack_from('<III', obj, sh+16)
            nrel = struct.unpack_from('<H', obj, sh+32)[0]
            if length < 32 or raw+length > len(obj):
                continue
            code = obj[raw:raw+length]
            mask = bytearray(b'\x01'*length)
            for j in range(nrel):
                addr, sym, kind = struct.unpack_from('<IIH', obj, relocs+10*j)
                width = 2 if kind in (1,2,10) else 4
                mask[addr:min(length,addr+width)] = b'\0'*max(0,min(width,length-addr))
            runs=[]; start=0
            for k in range(length+1):
                if k == length or not mask[k]:
                    if k>start: runs.append((start,k))
                    start=k+1
            if not runs: continue
            a,b=max(runs,key=lambda pair:pair[1]-pair[0])
            if b-a < 24: continue
            eligible += 1
            seek=0
            while True:
                found=exe.find(code[a:b],seek)
                if found<0: break
                base=found-a;seek=found+1
                if base>=0 and base+length<=len(exe) and all(not mask[k] or exe[base+k]==code[k] for k in range(length)):
                    hits.append(dict(member=member,section=i,length=length,file_offset=base,address=f'{va(base):08x}',symbols=[dict(name=s,address=f'{va(base)+v:08x}') for sec,v,s in symbols if sec==i+1]))
                    break
    results.append(dict(library=str(libpath),eligible_sections=eligible,matched_sections=len(hits),matched_bytes=sum(h['length'] for h in hits),matches=hits))
Path('analysis/crt-comparison.json').write_text(json.dumps(results,indent=2))
for r in results:
    print(r['library'],r['matched_sections'],'/',r['eligible_sections'],'sections;',r['matched_bytes'],'bytes')
