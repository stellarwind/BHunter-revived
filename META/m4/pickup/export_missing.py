"""Export only missing pinned original listings; never writes the reference EXE."""
import json
import subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[3]
OUT=Path(__file__).resolve().parent/'original'
PROGRAM=['--project','Bhunter','--projects-dir',str(ROOT/'ghidra-projects'),
         '--program','BHUNTER.EXE']
ADDRESSES=['004414d0','004558d0','00478f60','004776c0','00454fc0','00458300']

def query(*args):
    result=subprocess.run(['ghidra',*args,*PROGRAM],cwd=ROOT,
                          capture_output=True,text=True,check=True)
    return json.loads(result.stdout)

for address in ADDRESSES:
    listing=OUT/(address+'.tsv')
    decompiled=OUT/(address+'.c.txt')
    if listing.exists() and decompiled.exists():continue
    info=query('function','get','0x'+address)[0]
    start=int(address,16);size=info['size']
    rows=query('disasm','0x'+address,'-n',str(size))
    rows=[row for row in rows if start<=int(row['address'],16)<start+size]
    lines=['address\tbytes\tinstruction']
    cursor=start
    for row in rows:
        at=int(row['address'],16)
        if at!=cursor:raise RuntimeError(f'{address}: disassembly gap at {cursor:08x}, next {at:08x}')
        raw=row['bytes'];cursor+=len(raw)//2
        ins=row['mnemonic']
        if row['operands']:ins+=' '+','.join(row['operands'])
        lines.append(f"{row['address']}\t{raw}\t{ins}")
    if cursor!=start+size:raise RuntimeError(f'{address}: ended at {cursor:08x}, expected {start+size:08x}')
    code=query('decompile','0x'+address)[0]['code']
    listing.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    decompiled.write_text(code.replace('\r\n','\n').lstrip('\n').rstrip()+'\n',
                          encoding='utf-8',newline='\n')
    print(address,size,len(rows))
