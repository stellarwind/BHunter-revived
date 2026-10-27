"""Focused source-only split-projectile retirement check. No renderer or game execution."""
import hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
out=ROOT/'build/split-retirement-check';out.mkdir(parents=True,exist_ok=True)
evidence=ROOT/'META/m4/playable/split-retirement';evidence.mkdir(parents=True,exist_ok=True)
build=json.loads((ROOT/'build/game/build.json').read_text())
assert not build['debugging']
env=dict(os.environ)
for key in ('CL','_CL_','LINK','_LINK_'):env.pop(key,None)
vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC');sp3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc')
env['PATH']=os.pathsep.join([str(ROOT/'analysis/toolchain-sp3/bin'),'D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN',env.get('PATH','')])
env['INCLUDE']=os.pathsep.join([str(sp3/'include'),str(vc/'include')]);env['LIB']=os.pathsep.join([str(sp3/'lib'),str(vc/'lib')])
commands=[]
def run(args):
 p=subprocess.run(list(map(str,args)),cwd=out,env=env,text=True,capture_output=True)
 commands.append({'argv':list(map(str,args)),'exit_code':p.returncode,'output':p.stdout+p.stderr})
 if p.returncode:raise RuntimeError(p.stdout+p.stderr or str(args))
argv=next(c['argv'] for c in build['commands'] if c['argv'][-1].endswith('bh_effect_split_tick.c'))
argv=[a for a in argv[:-1] if not a.startswith('/Fo')]+['/DBH_EFFECT_SPLIT_STANDALONE','/Fo'+str(out/'check.obj'),str(ROOT/'tests/bh_effect_split.c')]
run(argv)
objects=[ROOT/'build/game'/(Path(name).stem+'.obj') for name in build['sources'] if name.endswith('.c') and not name.endswith('bh_game_main.c')]
run([vc/'bin/LINK.EXE','/nologo','/subsystem:console','/opt:ref','/out:'+str(out/'check.exe'),out/'check.obj',*objects,'D:/Decomp/dx7sdk/lib/ddraw.lib','D:/Decomp/dx7sdk/lib/dinput.lib','kernel32.lib','user32.lib','gdi32.lib','dsound.lib','winmm.lib','ole32.lib','advapi32.lib','comctl32.lib'])
p=subprocess.run([str(out/'check.exe')],cwd=out,env=env,text=True,capture_output=True)
commands.append({'argv':[str(out/'check.exe')],'exit_code':p.returncode,'output':p.stdout+p.stderr})
result={'passed':p.returncode==0,'scope':'Focused real split-projectile wall fixture; no game or renderer execution.','commands':commands}
(evidence/('after.json' if result['passed'] else 'before.json')).write_text(json.dumps(result,indent=2)+'\n')
print(p.stdout+p.stderr);raise SystemExit(p.returncode)
