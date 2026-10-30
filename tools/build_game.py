"""Build the standalone restored-source game. No test drivers or original code."""
import argparse,hashlib,json,os,shutil,subprocess,re
from prepare_sdl3 import prepare
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def stage_runtime(out):
 for folder, name in (('GAME_DATA', 'Bhunter.bin'), ('MUSIC_DATA', 'track02.wav')):
  destination=out/folder;destination.mkdir(exist_ok=True)
  source=ROOT/folder/name;target=destination/name
  if source.is_file() and source.resolve()!=target.resolve():
   if not target.exists() or (source.stat().st_size,source.stat().st_mtime_ns)!=(target.stat().st_size,target.stat().st_mtime_ns):
    shutil.copy2(source,target)
 for name in ('play-source.cmd','play-hardware.cmd','play-software.cmd'):
  (out/name).unlink(missing_ok=True)

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--compile-one');ap.add_argument('--incremental',action='store_true');ap.add_argument('--debugging',action='store_true',help='Compile optional diagnostics/profiling (DEBUGGING)');ap.add_argument('--mission-profile',action='store_true',help='Bounded in-memory mission markers without DEBUGGING');ap.add_argument('--frame-profile',action='store_true',help='Coarse frame/stage markers only; no per-entity timers or DEBUGGING');cd=ap.add_mutually_exclusive_group();cd.add_argument('--skip-cd-check',dest='skip_cd_check',action='store_true',help='Silently continue if neither CD audio nor local music is available (default)');cd.add_argument('--enable-cd-audio',dest='skip_cd_check',action='store_false',help='Enable original music failure warnings and retries; CD/local playback is always available');ap.set_defaults(skip_cd_check=True);ap.add_argument('--output-dir',type=Path,default=Path('build'),help='Self-contained runtime directory (default: build)');args=ap.parse_args()
 out=(ROOT/args.output_dir).resolve();out.mkdir(parents=True,exist_ok=True)
 cache_dir=ROOT/'build-cache'
 if out!=(ROOT/'build').resolve():cache_dir=cache_dir/'targets'/hashlib.sha256(str(out).encode()).hexdigest()[:12]
 cache_dir.mkdir(parents=True,exist_ok=True)
 vc=Path(os.environ.get('BH_VC_ROOT','D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC'));sp3=Path(os.environ.get('BH_SP3_ROOT','D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'));compiler=Path(os.environ.get('BH_COMPILER_DIR',ROOT/'toolchain/vc5/bin'));shared=Path(os.environ.get('BH_SHARED_DIR',vc.parent/'SHAREDIDE/BIN'));sdk=Path(os.environ.get('BH_DXSDK_ROOT','D:/Decomp/dx7sdk'))
 required=[compiler/'CL.EXE',compiler/'c1.dll',compiler/'c1xx.dll',compiler/'c2.exe',compiler/'MSDIS100.DLL',compiler/'MSPDB50.DLL',compiler/'MSVCP50.DLL',vc/'bin/LINK.EXE',sp3/'include/math.h',sdk/'include/ddraw.h',sdk/'lib/dinput.lib']
 missing=[str(p) for p in required if not p.is_file()]
 if missing:ap.error('Missing build dependencies:\n  '+'\n  '.join(missing)+'\nSee README.md for toolchain setup.')
 env=dict(os.environ)
 for n in ('CL','_CL_','LINK','_LINK_'):env.pop(n,None)
 env['PATH']=os.pathsep.join([str(compiler),str(shared),env.get('PATH','')]);env['INCLUDE']=os.pathsep.join([str(sp3/'include'),str(vc/'include')]);env['LIB']=os.pathsep.join([str(sp3/'lib'),str(vc/'lib')])
 record=dict(target=str(out/'BHunter-source.exe'),passed=False,debugging=args.debugging,mission_profile=args.mission_profile or args.frame_profile,frame_profile=args.frame_profile,skip_cd_audio_check=args.skip_cd_check,original_code_required=False,commands=[],sources={})
 objects_dir=cache_dir/'obj';objects_dir.mkdir(exist_ok=True)
 cache_path=objects_dir/'objects.json';cache=json.loads(cache_path.read_text()) if cache_path.exists() else {}
 headers=hashlib.sha256(b''.join(p.name.encode()+p.read_bytes() for p in sorted(list((ROOT/'src').glob('*.h'))+list((ROOT/'src').glob('*.inc'))))).hexdigest()
 def run(argv):
  p=subprocess.run([str(a) for a in argv],cwd=out,env=env,text=True,capture_output=True);record['commands'].append(dict(argv=[str(a) for a in argv],exit_code=p.returncode,output=p.stdout+p.stderr));(cache_dir/'build.json').write_text(json.dumps(record,indent=2)+'\n')
  if p.returncode:print(p.stdout+p.stderr);raise SystemExit(p.returncode)
 gpu_import=prepare(out)
 contexts=json.loads((ROOT/'tools/game_compile_context.json').read_text())['sources']
 sources=[ROOT/args.compile_one] if args.compile_one else [ROOT/'src'/name for name in sorted(set(contexts)|{'bh_game_app.c','bh_game_main.c'})];objects=[]
 for source in sources:
  obj=objects_dir/(source.stem+'.obj');objects.append(obj);record['sources'][source.relative_to(ROOT).as_posix()]=hashlib.sha256(source.read_bytes()).hexdigest()
  flags=contexts.get(source.name,['/I{DXSDK}/include','/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500','/DDIRECTINPUT_VERSION=0x0500'])
  profile_source=args.mission_profile or (args.frame_profile and source.name in {'bh_game_app.c','bh_frame_service.c','bh_mission_profile.c'})
  adapter_flags=['/DBH_SDL_BACKEND']
  if re.search(r'IDirect(?:Draw|3D)\w*_\w+\(|DirectDraw(?:Create|EnumerateA)\(', source.read_text()) or source.name in {'bh_menu_actions.c','bh_menu_edits.c'}:
   adapter_flags+=['/I'+str(sdk/'include'),'/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500','/FI'+str(ROOT/'src/bh_sdl_adapter.h')]
  argv=[compiler/'CL.EXE',*adapter_flags,'/nologo','/c','/ML','/O2','/G5','/Oi','/Gy',*(['/DDEBUGGING'] if args.debugging else []),*(['/DBH_MISSION_PROFILE'] if profile_source else []),*(['/DBH_SKIP_CD_AUDIO_CHECK'] if args.skip_cd_check and source.name=='bh_menu_transition.c' else []),*[a.replace('{DXSDK}',str(sdk)) for a in flags],'/Fo'+str(obj),source]
  signature=hashlib.sha256((headers+record['sources'][source.relative_to(ROOT).as_posix()]+repr([str(a) for a in argv])).encode()).hexdigest()
  entry=cache.get(source.name,{})
  if args.incremental and obj.exists() and entry.get('signature')==signature and entry.get('object_sha256')==hashlib.sha256(obj.read_bytes()).hexdigest():
   record['commands'].append(dict(argv=[str(a) for a in argv],exit_code=0,output='Reused verified object',signature=signature))
  else:
   run(argv);cache[source.name]=dict(signature=signature,object_sha256=hashlib.sha256(obj.read_bytes()).hexdigest())
   cache_path.write_text(json.dumps(cache,indent=2)+'\n')
 if args.compile_one:return
 for dependency in [ROOT/'src/bh_sdl_gpu.cpp',ROOT/'src/bh_sdl_adapter.h',ROOT/'src/shaders/legacy.hlsl',ROOT/'tools/prepare_sdl3.py']:
  record['sources'][dependency.relative_to(ROOT).as_posix()]=hashlib.sha256(dependency.read_bytes()).hexdigest()
 for include in (ROOT/'src').glob('*.inc'):record['sources'][include.relative_to(ROOT).as_posix()]=hashlib.sha256(include.read_bytes()).hexdigest()
 link=[vc/'bin/LINK.EXE','/nologo','/subsystem:windows','/opt:ref','/out:'+str(out/'BHunter-source.exe'),'/map:'+str(objects_dir/'BHunter-source.map'),*objects,gpu_import,sdk/'lib/dinput.lib','kernel32.lib','user32.lib','gdi32.lib','dsound.lib','winmm.lib','ole32.lib','advapi32.lib','comctl32.lib']
 run(link);stage_runtime(out);record['runtime_sha256']={name:hashlib.sha256((out/name).read_bytes()).hexdigest() for name in ('BHunterGPU.dll','SDL3.dll','shaders/legacy.hlsl')};record['passed']=True;record['translation_units']=len(sources)+1;record['executable_sha256']=hashlib.sha256((out/'BHunter-source.exe').read_bytes()).hexdigest();(cache_dir/'build.json').write_text(json.dumps(record,indent=2)+'\n');print('Built',out/'BHunter-source.exe','from',len(sources),'production translation units')
if __name__=='__main__':main()
