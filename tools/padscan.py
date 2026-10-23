"""VC5 declaration-context scanner, adapted from Squeezils padscan.sh.

Operates only on snapshots; never modifies or auto-installs original source.
Raw comparisons deliberately reject relocations. See tools/PADSCAN.md.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import uuid

ROOT = Path(__file__).resolve().parents[1]
# Reuse the inspected COFF reader already used by our verified math comparator.
sys.path.insert(0, str(ROOT/'analysis/astra-review'))
from targeted_probes import parse_coff

def sha(data): return hashlib.sha256(data).hexdigest()
def declarations(kind, count, members):
    lines=[]
    for i in range(count):
        n=f'BhPadscan{i:03}'
        if kind=='proto': lines.append(f'void {n}();')
        elif kind=='fwd': lines.append(f'class {n};')
        elif kind=='extern': lines.append(f'extern int g_{n};')
        else: lines.append(f'class {n} {{ public: '+''.join(f'void m{j}() {{}}' for j in range(members))+' };')
    return '\n'.join(lines)+'\n'

def place(source, extra, where):
    if where=='top': return extra+source
    if where=='eof': return source+'\n'+extra
    lines=source.splitlines(keepends=True)
    positions=[i for i,line in enumerate(lines) if re.match(r'\s*#\s*include\b',line)]
    at=positions[-1]+1 if positions else len(lines)
    return ''.join(lines[:at])+extra+''.join(lines[at:])

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--source',type=Path,action='append',required=True)
    ap.add_argument('--support',type=Path,action='append',default=[],help='Local header to snapshot beside sources; same basenames only')
    ap.add_argument('--targets',type=Path,required=True)
    ap.add_argument('--counts',type=int,nargs='+',default=[0,1,2,4,8])
    ap.add_argument('--kinds',choices=['proto','fwd','extern','shape'],nargs='+',default=['proto'])
    ap.add_argument('--placements',choices=['top','inc','eof'],nargs='+',default=['inc'])
    ap.add_argument('--members',type=int,default=4)
    args=ap.parse_args()
    if min(args.counts)<0 or args.members<0: ap.error('Counts must be nonnegative')
    if len(args.kinds)*len(args.placements)*len(set(args.counts))>128: ap.error('Bounded scan limit: 128 configurations')
    manifest=json.loads(args.targets.read_text())
    if not any(t.get('positive_control') for t in manifest['functions']):
        raise ValueError('Declare at least one positive_control target in the manifest')
    original=ROOT/'GAME_DATA/BHUNTER.EXE';exe=original.read_bytes()
    if sha(exe)!=manifest['executable_sha256']: raise ValueError('Reference EXE hash mismatch')
    pe=struct.unpack_from('<I',exe,60)[0];opt=pe+24
    imagebase=struct.unpack_from('<I',exe,opt+28)[0];sections=[]
    for i in range(struct.unpack_from('<H',exe,pe+6)[0]):
        sh=opt+struct.unpack_from('<H',exe,pe+20)[0]+40*i
        vs,rva,rs,raw=struct.unpack_from('<IIII',exe,sh+8)
        sections.append((imagebase+rva,rs,raw))
    targets=[]
    for t in manifest['functions']:
        va=int(t['address'],0);size=t['size']
        start=next(raw+va-base for base,n,raw in sections if base<=va and va+size<=base+n)
        targets.append((t,exe[start:start+size]))
    sources=[p.resolve() for p in args.source]
    supports=[p.resolve() for p in args.support]
    names=[p.name.lower() for p in sources+supports]
    if len(names)!=len(set(names)): raise ValueError('Snapshot filenames must be unique')
    originals={p:p.read_bytes() for p in sources+supports}
    run=ROOT/'analysis/padscan'/('run-'+uuid.uuid4().hex[:12])
    work=run/'work';work.mkdir(parents=True)
    for p,data in originals.items(): (work/p.name).write_bytes(data)
    tools=ROOT/'analysis/toolchain-sp3/bin'
    include=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc/include')
    flags=['/nologo','/c','/ML','/Gy','/TP','/O2','/G5','/I'+str(include)]
    env=os.environ.copy();env.pop('CL',None);env.pop('_CL_',None)
    env['PATH']=str(tools)+os.pathsep+env['PATH'];env['INCLUDE']=str(include)
    report=dict(upstream=str(ROOT/'tools/upstream/squeezil/padscan.sh'),
                upstream_sha256=sha((ROOT/'tools/upstream/squeezil/padscan.sh').read_bytes()),
                flags=flags,exe_sha256=sha(exe),sources={str(p):sha(d) for p,d in originals.items()},
                tools={f:sha((tools/f).read_bytes()) for f in ['CL.EXE','c1.dll','c1xx.dll','c2.exe']},
                math_header_sha256=sha((include/'math.h').read_bytes()),variants=[],notes=[
                'Snapshots preserve basenames and one stable absolute source path throughout a scan.',
                'The snapshot parent differs from the input parent; baseline must be checked in this context.',
                'Raw matcher rejects target relocations; no arbitrary masking or automatic source installation.',
                'Local include dependencies must be supplied explicitly using --support (flat snapshot).'])
    variants=[('baseline','inc',0)]+[(k,a,n) for k,a,n in itertools.product(args.kinds,args.placements,sorted(set(args.counts))) if n]
    unique={}
    try:
      for kind,at,n in variants:
        label=f'{kind}-{at}-{n}';trial=run/label;trial.mkdir()
        objects=[];commands=[]
        for i,p in enumerate(sources):
            text=originals[p].decode('utf-8-sig')
            modified=text if not n else place(text,declarations(kind,n,args.members),at)
            snapshot=work/p.name;snapshot.write_text(modified,encoding='utf-8')
            shutil.copy2(snapshot,trial/p.name)
            obj=work/f'unit{i}.obj';command=[str(tools/'CL.EXE'),*flags,'/Fo'+str(obj),str(snapshot)]
            process=subprocess.run(command,cwd=work,env=env,capture_output=True,text=True,timeout=60)
            (trial/f'compile{i}.log').write_text(process.stdout+process.stderr)
            commands.append(command)
            if process.returncode: raise RuntimeError(f'Compile failed: {label}, {p.name}; see {trial}')
            saved=trial/f'unit{i}.obj';shutil.copy2(obj,saved);objects.append(parse_coff(saved.read_bytes()))
        rows=[]
        for target,wanted in targets:
            found=[]
            for symbols,secs in objects:
                for s in symbols.values():
                    if s['name']==target['symbol'] and s['section']>0:
                        sec=secs[s['section']];start=s['value']
                        nextstarts=[v['value'] for v in symbols.values() if v['section']==s['section'] and v['type']&0x20 and v['value']>start]
                        end=min(nextstarts) if nextstarts else len(sec['data'])
                        code=sec['data'][start:end]
                        rel=[r for r in sec['relocations'] if start<=r['offset']<end]
                        found.append((code,rel))
            if len(found)!=1: raise ValueError(f"Expected one definition of {target['symbol']}, got {len(found)}")
            code,rel=found[0];length=len(wanted)
            same=sum(a==b for a,b in zip(code,wanted))
            valid_tail=len(code)>=length and len(code)-length<=15 and all(v in (0x90,0xcc,0) for v in code[length:])
            exact=not rel and valid_tail and code[:length]==wanted
            key=sha(code+json.dumps(rel,sort_keys=True).encode())
            unique.setdefault(target['symbol'],{}).setdefault(key,[]).append(label)
            rows.append(dict(symbol=target['symbol'],address=target['address'],raw_match=exact,positive_control=target.get('positive_control',False),
                             target_bytes=length,object_bytes=len(code),different_positions=length-same,
                             generated_hex=code.hex(),original_hex=wanted.hex(),relocations=rel,fingerprint=key))
        controls_pass=all(r['raw_match'] for r in rows if r['positive_control'])
        report['variants'].append(dict(label=label,commands=commands,functions=rows,controls_pass=controls_pass))
        print(label,'raw matches',sum(r['raw_match'] for r in rows),'/',len(rows),'controls_pass='+str(controls_pass))
        if kind=='baseline' and not controls_pass: raise RuntimeError('Positive control failed in snapshot; do not interpret the scan')
    finally:
        report['unique_outputs']=unique
        report['inputs_unchanged']=all(p.read_bytes()==data for p,data in originals.items())
        report['reference_unchanged']=original.read_bytes()==exe
        (run/'results.json').write_text(json.dumps(report,indent=2))
        for p,data in originals.items(): (work/p.name).write_bytes(data)
        if not report['inputs_unchanged'] or not report['reference_unchanged']: raise RuntimeError('Input changed during scan')
        print('Evidence:',run/'results.json')
    print('Distinct outputs:',{symbol:len(patterns) for symbol,patterns in unique.items()})

if __name__=='__main__': main()
