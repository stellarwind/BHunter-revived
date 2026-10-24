"""Build/run the isolated ten-pure-function numerical oracle; never a game loader."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
from m0 import ROOT,MANIFEST,digest,write


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--label',default='current')
    parser.add_argument('--golden',type=Path,help='Explicit destination for new original-derived PC53 numerical fixtures')
    args=parser.parse_args()
    if not args.label.replace('-','').replace('_','').isalnum():raise RuntimeError('Use a simple evidence label')
    roots=dict(repo=ROOT,fixtures=ROOT,
        compiler=Path(os.environ.get('BH_COMPILER_DIR',ROOT/'analysis/toolchain-sp3/bin')),
        vc=Path(os.environ.get('BH_VC_ROOT','D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC')),
        sp3=Path(os.environ.get('BH_SP3_ROOT','D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc')),
        shared=Path(os.environ.get('BH_SHARED_DIR','D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN')))
    manifest=json.loads(MANIFEST.read_text())
    for entry in manifest['files']:
        if entry['root']=='repo' and entry['path']!='GAME_DATA/BHUNTER.EXE':continue
        path=roots[entry['root']]/entry['path']
        if digest(path)!=entry['sha256']:raise RuntimeError('Pinned input changed: '+str(path))
    out=ROOT/'build/math-oracle'/args.label;out.mkdir(parents=True,exist_ok=True)
    report=dict(target='bhunter-math-oracle.exe',adapter='unchanged pinned analysis/math/differential.c mapper; same ten math functions only',
        exe_sha256=digest(ROOT/'GAME_DATA/BHUNTER.EXE'),sources={p:digest(ROOT/p) for p in ['src/bh_math.c','src/bh_math.h','tests/math_cases.c','tests/math_cases.h','tests/math_oracle.c','analysis/math/differential.c']},commands=[],passed=False)
    env=os.environ.copy()
    for name in ('CL','_CL_','LINK','_LINK_'):env.pop(name,None)
    env['PATH']=os.pathsep.join([str(roots['compiler']),str(roots['shared']),env.get('PATH','')])
    env['INCLUDE']=os.pathsep.join(str(roots[k]/'include') for k in ('sp3','vc'))
    env['LIB']=os.pathsep.join(str(roots[k]/'lib') for k in ('sp3','vc'))
    report['environment']={k:env[k] for k in ('INCLUDE','LIB')}
    report['dependency_manifest_sha256']=digest(MANIFEST)
    def run(label,command,required=True):
        p=subprocess.run([str(a) for a in command],cwd=out,env=env,capture_output=True,text=True)
        report['commands'].append(dict(label=label,argv=[str(a) for a in command],exit_code=p.returncode,output=p.stdout+p.stderr))
        (out/(label+'.txt')).write_text(p.stdout+p.stderr)
        if required and p.returncode:raise RuntimeError(p.stdout+p.stderr)
        return p
    try:
        objects=[]
        for name in ('src/bh_math.c','tests/math_cases.c','tests/math_oracle.c'):
            obj=out/(Path(name).stem+'.obj');objects.append(obj)
            run('compile-'+Path(name).stem,[roots['compiler']/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/Fo'+str(obj),ROOT/name])
        run('link',[roots['vc']/'bin/LINK.EXE','/nologo','/base:0x10000000','/fixed','/subsystem:console','/map:'+str(out/'oracle.map'),'/out:'+str(out/'bhunter-math-oracle.exe'),*objects,'kernel32.lib'])
        p=run('scenario',[out/'bhunter-math-oracle.exe',ROOT/'GAME_DATA/BHUNTER.EXE',*([args.golden.resolve()] if args.golden else [])],False)
        report['results']=[json.loads(line) for line in p.stdout.splitlines() if line.startswith('{')]
        report['counterexamples']=[line for line in p.stdout.splitlines() if line.startswith('FIRST')]
        report['passed']=p.returncode==0
        if digest(ROOT/'GAME_DATA/BHUNTER.EXE')!=report['exe_sha256']:raise RuntimeError('Reference changed')
        for mode in range(3):
            rows=[r for r in report['results'] if r.get('mode')==mode]
            print(json.dumps(dict(mode=mode,**{key:sum(r[key] for r in rows) for key in ['cases','return_differences','finite_word_differences','classification_or_infinity_differences','nan_payload_differences','fp_status_differences','guard_or_control_failures']})))
        print(json.dumps(report['results'][-1]))
        if args.golden:report['golden']=dict(path=str(args.golden),sha256=digest(args.golden.resolve()))
    finally:
        write(ROOT/'META/m2/math'/('oracle-'+args.label+'.json'),report)
    return 0 if report['passed'] else 1


if __name__=='__main__':sys.exit(main())
