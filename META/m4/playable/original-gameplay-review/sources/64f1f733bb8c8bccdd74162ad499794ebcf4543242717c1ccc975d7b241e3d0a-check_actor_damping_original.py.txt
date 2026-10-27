"""Isolated original 00439910 arithmetic-block oracle, never a game loader.

Copies only 00439a52..00439ba7 and its six read-only constants. The wrapper
supplies ESI and scratch stack; only absolute constant operands are relocated.
No imports, calls, game globals or original subsystem execution are admitted.
"""
import os
import struct
import subprocess
from pathlib import Path
from record_pickup_basic import ROOT, reference, read, write, sha

def main():
    _, raw = reference()
    out = ROOT/'build/original-review'
    out.mkdir(exist_ok=True)
    start,end=0x439a52,0x439ba7
    rows=read(ROOT/'META/m4/target656/response/original/00439910.disasm.json')
    rows=[r for r in rows if start<=int(r['address'],16)<end]
    body=bytearray(); reloc=[]; constants={}
    for r in rows:
        at=int(r['address'],16); b=bytes.fromhex(r['bytes'])
        assert at==start+len(body) and raw(at,len(b))==b
        assert r['mnemonic']!='CALL'
        for operand in r['operands']:
            if '[0x00' in operand:
                address=int(operand.split('[0x')[1].split(']')[0],16)
                size=8 if 'double' in operand else 4
                constants.setdefault(address,raw(address,size))
                pos=b.index(struct.pack('<I',address))
                reloc.append((len(body)+pos,address))
        body.extend(b)
    assert len(body)==end-start
    # Compare the second owner's arithmetic instructions, not its decompiler
    # expression. Exclude its interleaved health decay and integer tick loads.
    damage=read(ROOT/'META/m4/target600/b3/original/004357d0.disasm.json')
    damage=[r for r in damage if 0x4357e1<=int(r['address'],16)<0x435950 and
            not 0x435813<=int(r['address'],16)<0x435825 and r['mnemonic'] not in ('MOV','SUB')]
    def normalized(items):
        return [(r['mnemonic'],[] if r['mnemonic']=='JZ' else r['operands']) for r in items if r['mnemonic']!='CMP']
    assert normalized(rows)==normalized(damage)
    for r in damage:assert raw(int(r['address'],16),len(bytes.fromhex(r['bytes'])))==bytes.fromhex(r['bytes'])
    # push esi; sub esp,40; mov esi,[esp+48]; copied block; restore; ret.
    prefix=bytes.fromhex('5683ec408b742448')
    code=prefix+body+bytes.fromhex('83c4405ec3')
    header='static unsigned char original_bytes[]={'+','.join(str(b) for b in code)+'};\n'
    header+='static void relocate(unsigned char *p){unsigned long address;\n'
    for i,(address,data) in enumerate(constants.items()):
        offset=0x300+i*8
        header+='{'+'unsigned char data[]={'+','.join(str(b) for b in data)+'};memcpy(p+%d,data,sizeof(data));}\n'%offset
        for pos,a in reloc:
            if a==address:header+='address=(unsigned long)(p+%d);memcpy(p+%d,&address,4);\n'%(offset,len(prefix)+pos)
    header+='}\n'
    (out/'actor_damping_original.h').write_text(header)
    base=read(ROOT/'build/runtime-fixes/validation.json')
    env=dict(os.environ)
    vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC'
    sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
    env['PATH']=str(ROOT/'analysis/toolchain-sp3/bin')+';D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;'+env['PATH']
    env['INCLUDE']=sp+'/include;'+vc+'/include';env['LIB']=sp+'/lib;'+vc+'/lib'
    for key in ('CL','_CL_','LINK','_LINK_'):env.pop(key,None)
    obj=out/'actor_damping_original.obj';exe=out/'actor_damping_original.exe'
    compile=list(next(c['argv'] for c in base['commands'] if Path(c['argv'][-1]).name=='actor_response.c'))
    compile[-1]=str(ROOT/'tests/actor_damping_original.c')
    compile=[('/Fo'+str(obj)) if a.startswith('/Fo') else a for a in compile]
    subprocess.run(compile,env=env,cwd=out,check=True)
    link=next(c['argv'] for c in base['commands'] if 'LINK.EXE' in c['argv'][0])
    rsp=(ROOT/'build/runtime-fixes/link.rsp').read_text().splitlines()
    rsp=[a for a in rsp if 'runtime_fixes.obj' not in a and not a.startswith('"/out:') and not a.startswith('"/map:')]
    rsp+=['"/out:'+str(exe)+'"','"'+str(obj)+'"']
    (out/'oracle-link.rsp').write_text('\n'.join(rsp))
    subprocess.run([link[0],'@'+str(out/'oracle-link.rsp')],env=env,cwd=out,check=True)
    result=subprocess.run([str(exe)],cwd=out,text=True,capture_output=True)
    (out/'actor-damping.log').write_text(result.stdout+result.stderr)
    write(out/'actor-damping.json',dict(passed=result.returncode==0,original_code_executed=True,
        scope='Isolated arithmetic block 00439a52..00439ba7, not whole function or game',
        identical_second_owner_arithmetic='004357e1..0043594e; health decay and integer tick instructions excluded',
        relocations=reloc,constants={hex(a):b.hex() for a,b in constants.items()},
        source_sha256=sha(ROOT/'src/bh_actor_response.c'),output=result.stdout,exit_code=result.returncode))
    print(result.stdout)
    return result.returncode

if __name__=='__main__':raise SystemExit(main())
