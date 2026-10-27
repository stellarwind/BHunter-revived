"""Isolated call-free firing arithmetic; never linked into the game."""
import os, struct, subprocess
from record_pickup_basic import ROOT, reference, read, write, sha

def main():
    ledger,raw=reference();out=ROOT/'build/launch-review';out.mkdir(exist_ok=True)
    assert raw(0x4892a0,8)==struct.pack('<d',16384.0)
    assert raw(0x4895e8,8)==struct.pack('<d',-1.0/8192)
    start,end=0x430627,0x430731
    rows=read(ROOT/'META/m4/target600/a5/original/004305f0.disasm.json')
    rows=[r for r in rows if start<=int(r['address'],16)<end]
    body=bytearray();reloc=[];constants={}
    for row in rows:
        at=int(row['address'],16);b=bytes.fromhex(row['bytes'])
        assert at==start+len(body) and raw(at,len(b))==b and row['mnemonic']!='CALL'
        for operand in row['operands']:
            if '[0x00' in operand:
                address=int(operand.split('[0x')[1].split(']')[0],16)
                constants[address]=raw(address,8 if 'double' in operand else 4)
                reloc.append((len(body)+b.index(struct.pack('<I',address)),address))
        body.extend(b)
    assert len(body)==end-start
    # Save ESI/EDI, reserve original scratch, load actor and phase. Original
    # relative branches stay internal; no imports, RNG or game globals execute.
    prefix=bytes.fromhex('565783ec408b74244c8b4c2454')
    suffix=bytes.fromhex('8b7c24508b44242c89078b4424308947048b4424348947088b44241089470c8b4424148947108b4424248947148b44242889471883c4405f5ec3')
    code=prefix+body+suffix
    header='static unsigned char original_bytes[]={'+','.join(map(str,code))+'};\nstatic void relocate(unsigned char*p){unsigned long a;\n'
    for i,(address,data) in enumerate(constants.items()):
        offset=0x300+i*8
        header+='{unsigned char d[]={'+','.join(map(str,data))+'};memcpy(p+%d,d,sizeof(d));}\n'%offset
        for pos,a in reloc:
            if a==address:header+='a=(unsigned long)(p+%d);memcpy(p+%d,&a,4);\n'%(offset,len(prefix)+pos)
    header+='}\n';(out/'original.h').write_text(header)
    env=dict(os.environ);vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC';sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
    env['PATH']=str(ROOT/'analysis/toolchain-sp3/bin')+';D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;'+env['PATH']
    env['INCLUDE']=sp+'/include;'+vc+'/include';env['LIB']=sp+'/lib;'+vc+'/lib'
    for k in ('CL','_CL_','LINK','_LINK_'):env.pop(k,None)
    commands=[[str(ROOT/'analysis/toolchain-sp3/bin/CL.EXE'),'/nologo','/c','/ML','/O2','/G5','/Fo'+str(out/'oracle.obj'),str(ROOT/'tests/ballistic_launch_original.c')],
              [vc+'/bin/LINK.EXE','/nologo','/subsystem:console','/out:'+str(out/'oracle.exe'),str(out/'oracle.obj'),'kernel32.lib']]
    for cmd in commands:subprocess.run(cmd,cwd=out,env=env,check=True)
    p=subprocess.run([str(out/'oracle.exe')],cwd=out,capture_output=True,text=True,timeout=15)
    write(out/'original.json',dict(passed=p.returncode==0,scope='Original call-free 00430627..00430731 muzzle and pre-spread velocity under PC53; spread uses byte-verified constants and original VC5 rand, not copied game instructions',
       role='game-owned ballistic firing arithmetic within 004305f0; not an independent owner',reference_exe_sha256=ledger['reference_exe_sha256'],instructions=rows,constants={hex(a):b.hex() for a,b in constants.items()},relocations=reloc,
       original_code_executed=True,shipping_dependency=False,commands=commands,output=p.stdout,exit_code=p.returncode,golden_sha256=sha(out/'ballistic_launch_golden.h')))
    print(p.stdout);return p.returncode
if __name__=='__main__':raise SystemExit(main())
