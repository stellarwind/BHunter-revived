"""Real source/native text ring integration; no original code is loaded."""
import ast,hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];FOLDER=Path(__file__).resolve().parent;OUT=ROOT/'build/text-queue'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    tree=ast.parse((ROOT/'tools/m0.py').read_text());modules=None
    for node in ast.walk(tree):
        if isinstance(node,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='modules' for t in node.targets):modules=ast.literal_eval(node.value)
    assert modules
    sources=sorted({s for s,o in modules if s.startswith('src/')}|{'src/bh_text_queue.c','src/bh_atlas_text.c'})+['tests/text_queue.c']
    headers=['src/bh_text_queue.h','src/bh_atlas_text.h','src/bh_atlas_glyph.h','src/bh_atlas_page.h','src/bh_atlas_image.h','src/bh_texture.h']
    compiler=ROOT/'analysis/toolchain-sp3/bin';vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC')
    sp3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc');sdk=Path('D:/Decomp/dx7sdk');OUT.mkdir(exist_ok=True,parents=True)
    env=os.environ.copy()
    for k in ('CL','_CL_','LINK','_LINK_'):env.pop(k,None)
    env['PATH']=os.pathsep.join([str(compiler),str(vc.parent/'SHAREDIDE/BIN'),env.get('PATH','')])
    env['INCLUDE']=os.pathsep.join(str(p/'include') for p in (sp3,vc));env['LIB']=os.pathsep.join(str(p/'lib') for p in (sp3,vc))
    result=dict(target='text-queue.exe',passed=False,original_code_required=False,game_launches=0,
                sources={s:sha(ROOT/s) for s in sources+headers},commands=[])
    def run(label,args):
        p=subprocess.run([str(x) for x in args],cwd=ROOT,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        result['commands'].append(dict(label=label,argv=[str(x) for x in args],returncode=p.returncode,output=p.stdout))
        if p.returncode:raise RuntimeError(label+': '+p.stdout)
        return p.stdout
    try:
        objects=[]
        for source in sources:
            obj=OUT/(Path(source).stem+'.obj');objects.append(obj)
            flags=['/DBH_TEXT_QUEUE_STANDALONE'] if source=='tests/text_queue.c' else []
            run('compile-'+Path(source).stem,[compiler/'cl.exe','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy',
                '/I'+str(sdk/'include'),'/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500',*flags,'/Fo'+str(obj),ROOT/source])
        run('link',[vc/'bin/LINK.EXE','/nologo','/subsystem:console','/verbose',
            '/map:'+str(OUT/'text-queue.map'),'/out:'+str(OUT/'text-queue.exe'),*objects,
            sdk/'lib/ddraw.lib','kernel32.lib','dsound.lib','user32.lib','ole32.lib'])
        output=run('actual-native-source-checks',[OUT/'text-queue.exe']);print(output,end='')
        result['scenario']=json.loads(output.splitlines()[-1]);assert result['scenario']['checks']>0 and not result['scenario']['failures']
        result['executable_sha256']=sha(OUT/'text-queue.exe');result['passed']=True
        (FOLDER/'standalone-link-map.txt').write_bytes((OUT/'text-queue.map').read_bytes())
    finally:(FOLDER/'standalone-build.json').write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
