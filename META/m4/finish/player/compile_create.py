import hashlib,json,os,subprocess,shutil
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];OUT=ROOT/'build/player';OUT.mkdir(parents=True,exist_ok=True)
vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC');sp3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc');compiler=ROOT/'analysis/toolchain-sp3/bin';shared=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN')
env=dict(os.environ)
for name in ('CL','_CL_','LINK','_LINK_'):env.pop(name,None)
env['PATH']=os.pathsep.join([str(compiler),str(shared),env.get('PATH','')]);env['INCLUDE']=os.pathsep.join([str(sp3/'include'),str(vc/'include')]);env['LIB']=os.pathsep.join([str(sp3/'lib'),str(vc/'lib')])

p= subprocess.run([str(compiler/"CL.EXE"),"/nologo","/c","/ML","/O2","/G5","/Oi","/Gy","/ID:/Decomp/dx7sdk/include","/DDIRECT3D_VERSION=0x0500","/DDIRECTDRAW_VERSION=0x0500","/Fo"+str(OUT/"bh_player_create.obj"),str(ROOT/"src/bh_player_create.c")],env=env,cwd=OUT,capture_output=True,text=True);print(p.stdout+p.stderr);raise SystemExit(p.returncode)
