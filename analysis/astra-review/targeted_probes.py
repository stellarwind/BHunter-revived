"""Reproducible small-function compiler experiments. No original file modifications."""
from pathlib import Path
import hashlib,itertools,json,os,struct,subprocess,sys

ROOT=Path(__file__).resolve().parents[2]
OUT=Path(__file__).resolve().parent
EXE=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
assert hashlib.sha256(EXE).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
TARGETS={'translate':(0x401460,45,[]),'normalize':(0x4011f0,113,[53,72]),'normalize_to':(0x401270,111,[53,72])}

def parse_coff(b):
    symptr,nsym=struct.unpack_from('<II',b,8);st=symptr+nsym*18
    symbols={};i=0
    while i<nsym:
        at=symptr+18*i;raw=b[at:at+8]
        if raw[:4]==b'\0'*4:
            z=st+struct.unpack_from('<I',raw,4)[0];name=b[z:].split(b'\0')[0].decode()
        else:name=raw.split(b'\0')[0].decode()
        value,section,typ,storage,aux=struct.unpack_from('<IhHBB',b,at+8)
        symbols[i]=dict(name=name,value=value,section=section,type=typ,storage=storage)
        i+=1+aux
    sections={}
    for n in range(1,struct.unpack_from('<H',b,2)[0]+1):
        sh=20+(n-1)*40;size,rawptr,relptr=struct.unpack_from('<III',b,sh+16);nr=struct.unpack_from('<H',b,sh+32)[0]
        rel=[]
        for j in range(nr):
            addr,sym,kind=struct.unpack_from('<IIH',b,relptr+10*j)
            rel.append(dict(offset=addr,symbol_index=sym,kind=kind,symbol=symbols[sym]))
        sections[n]=dict(data=b[rawptr:rawptr+size],relocations=rel)
    return symbols,sections

def describe_reloc(rel,sections):
    s=rel['symbol'];sec=s['section'];value=s['value']
    r=dict(rel)
    if sec>0:r['referent_hex']=sections[sec]['data'][value:value+16].hex()
    return r

def run(name,bodies,configs,target='translate',mode='sp3',verbose=True,preamble='',signature_override=None):
    folder=OUT/name;folder.mkdir(exist_ok=True)
    bindir=ROOT/('analysis/toolchain-'+mode)/'bin'
    env=os.environ.copy();env.pop('CL',None);env.pop('_CL_',None);env['PATH']=str(bindir)+os.pathsep+env['PATH']
    va,length,target_relocs=TARGETS[target];original=EXE[va-0x400c00:va-0x400c00+length]
    for offset,opcode in zip(target_relocs,[bytes.fromhex('dc15'),bytes.fromhex('dc3d')]):
        assert original[offset-2:offset]==opcode
        assert struct.unpack_from('<I',original,offset)[0] in (0x4891e0,0x4891e8)
    signature={'translate':'float x,float y,float z,float *m','normalize':'float *v','normalize_to':'float *v,float *out'}[target]
    if signature_override is not None:signature=signature_override
    ret='void' if target=='translate' else 'int'
    header='#ifdef __cplusplus\nextern "C" {\n#endif\n'
    if target!='translate':header+='double __cdecl sqrt(double);\n'
    source=preamble+header+''.join(f'{ret} {n}({signature}){{{body}}}\n' for n,body in bodies.items())+'#ifdef __cplusplus\n}\n#endif\n'
    sourcepath=folder/'candidates.c';sourcepath.write_text(source)
    (folder/'bodies.json').write_text(json.dumps(bodies,indent=2))
    manifest=dict(target=target,original_hex=original.hex(),original_va=hex(va),original_length=length,original_relocation_offsets=target_relocs,toolchain=mode,tools={},builds=[])
    for file in ['CL.EXE','c1.dll','c1xx.dll','c2.exe','c1.exe','c1xx.exe']:
        p=bindir/file
        if p.exists():manifest['tools'][str(p)]=hashlib.sha256(p.read_bytes()).hexdigest()
    unique={};matches=[]
    for ci,flags in enumerate(configs):
        obj=folder/f'build-{ci:03}.obj';command=[str(bindir/'CL.EXE'),'/nologo','/c','/ML','/Gy',*flags,'/Fo'+str(obj),str(sourcepath)]
        p=subprocess.run(command,cwd=folder,env=env,capture_output=True,text=True)
        (folder/f'build-{ci:03}.log').write_text(p.stdout+p.stderr)
        assert p.returncode==0,(command,p.stdout,p.stderr)
        symbols,sections=parse_coff(obj.read_bytes());count=0
        for symbol in symbols.values():
            n=symbol['name'].lstrip('_')
            if n not in bodies or symbol['section']<=0:continue
            count+=1;section=sections[symbol['section']];value=symbol['value'];code=section['data'][value:]
            relocs=[describe_reloc(r,sections) for r in section['relocations'] if r['offset']>=value]
            for r in relocs:r['offset']-=value
            mask=bytearray(b'\xff'*len(code))
            for r in relocs:
                assert r['kind'] in (6,20),r
                assert 0<=r['offset']<=len(code)-4,r
                mask[r['offset']:r['offset']+4]=b'\0'*4
            prefix=next((i for i,(x,y) in enumerate(zip(code,original)) if mask[i] and x!=y),min(len(code),length))
            valid_relocs=sorted(r['offset'] for r in relocs)==target_relocs
            exact=(len(code)>=length and prefix==length and all(x in (0,0x90,0xcc) for x in code[length:]) and valid_relocs)
            normalized=bytes(c&m for c,m in zip(code,mask))
            key=hashlib.sha256(normalized+json.dumps([(r['offset'],r['kind'],r['symbol']['name']) for r in relocs]).encode()).hexdigest()[:20]
            variant=dict(build=ci,name=n,flags=flags)
            if key not in unique:unique[key]=dict(code_hex=code.hex(),mask_hex=mask.hex(),relocations=relocs,prefix=prefix,exact=exact,variants=[])
            unique[key]['variants'].append(variant)
            if exact:
                matches.append(variant)
                if verbose:print('MATCH',mode,target,variant,flush=True)
        assert count==len(bodies),(count,len(bodies))
        manifest['builds'].append(dict(id=ci,command=command,cwd=str(folder),returncode=p.returncode,object_sha256=hashlib.sha256(obj.read_bytes()).hexdigest(),functions=count))
    (folder/'manifest.json').write_text(json.dumps(manifest,indent=2))
    (folder/'unique.json').write_text(json.dumps(unique,indent=2))
    ranking=sorted(unique.values(),key=lambda r:r['prefix'],reverse=True)
    summary=dict(target=target,toolchain=mode,builds=len(configs),functions=len(bodies),results=len(configs)*len(bodies),unique=len(unique),matches=matches,best_prefix=ranking[0]['prefix'])
    (folder/'summary.json').write_text(json.dumps(summary,indent=2))
    if verbose:
        print(json.dumps(summary),flush=True)
        for r in ranking[:5]:print('BEST',r['prefix'],r['code_hex'],r['variants'][0],flush=True)
    return unique

def promotion_bodies():
    bodies={}
    for ca,cm,flip in itertools.product([False,True],repeat=3):
        prefix=f'p{int(ca)}{int(cm)}{int(flip)}'
        args=['x','y','z'];loc=['a','b','c'];offs=[3,7,11]
        expr=[]
        for a,o in zip(args,offs):
            l=f'(double){a}' if ca else a;r=f'(double)m[{o}]' if cm else f'm[{o}]'
            expr.append(f'{r}+{l}' if flip else f'{l}+{r}')
        bodies[prefix+'_direct']=''.join(f'm[{o}]=(float)({e});' for o,e in zip(offs,expr))
        for typ in ['float','double']:
            bodies[prefix+'_'+typ+'_results']=''.join(f'{typ} {v}={e};' for v,e in zip(loc,expr))+''.join(f'm[{o}]=(float){v};' for o,v in zip(offs,loc))
        bodies[prefix+'_arg_update']=''.join(f'{a}=(float)({e});' for a,e in zip(args,expr))+''.join(f'm[{o}]={a};' for o,a in zip(offs,args))
    for at,mt,order in itertools.product(['float','double'],['float','double'],[0,1]):
        adecl=''.join(f'{at} {a}={v};' for a,v in zip(['a','b','c'],['x','y','z']))
        mdecl=''.join(f'{mt} {a}=m[{v}];' for a,v in zip(['d','e','f'],[3,7,11]))
        bodies[f'captured_{at}_{mt}_{order}']=(adecl+mdecl if order==0 else mdecl+adecl)+''.join(f'm[{o}]=(float)({a}+{b});' for o,a,b in zip([3,7,11],['a','b','c'],['d','e','f']))
    return bodies

def mixed_promotion_bodies():
    bodies={}
    # Each component independently chooses float arithmetic, either operand
    # promoted, or both promoted, with both commutative source orders.
    for choices in itertools.product(range(8),repeat=3):
        expr=[]
        for a,o,ch in zip(['x','y','z'],[3,7,11],choices):
            l=f'(double){a}' if ch&1 else a
            r=f'(double)m[{o}]' if ch&2 else f'm[{o}]'
            expr.append(f'{r}+{l}' if ch&4 else f'{l}+{r}')
        bodies['mixed_'+''.join(map(str,choices))]=''.join(f'm[{o}]=(float)({e});' for o,e in zip([3,7,11],expr))
    return bodies

def normalization_bodies(to=False):
    bodies={}
    # Distinguish captured component types, sum association/order, and length
    # representation; sqrt is declared with its real double signature.
    for capture,order,group,length in itertools.product(['none','float','double'],itertools.permutations(range(3)),['left','right'],['float','double']):
        vals=[f'v[{i}]' for i in range(3)] if capture=='none' else ['x','y','z']
        decl='' if capture=='none' else ''.join(f'{capture} {v}=v[{i}];' for i,v in enumerate(vals))
        p=[f'{vals[i]}*{vals[i]}' for i in order]
        s=f'({p[0]}+{p[1]})+{p[2]}' if group=='left' else f'{p[0]}+({p[1]}+{p[2]})'
        dest='out' if to else 'v'
        body=decl+f'{length} n=({length})sqrt({s});if(n==0.0)return 0;n=({length})(1.0/n);'+''.join(f'{dest}[{i}]=(float)(n*v[{i}]);' for i in range(3))+'return 1;'
        bodies[f'n_{capture}_{"".join(map(str,order))}_{group}_{length}']=body
    return bodies

def normalization_promoted_bodies(to=False):
    bodies={};dest='out' if to else 'v'
    for squares,length,scale in itertools.product(itertools.product(range(4),repeat=3),['float','double'],range(4)):
        products=[]
        for i,ch in enumerate(squares):
            a=f'(double)v[{i}]' if ch&1 else f'v[{i}]'
            b=f'(double)v[{i}]' if ch&2 else f'v[{i}]'
            products.append(f'({a}*{b})')
        # This source association corresponds to the observed original sum.
        s=f'({products[1]}+{products[2]})+{products[0]}'
        body=f'{length} n=({length})sqrt({s});if(n==0.0)return 0;n=({length})(1.0/n);'
        for i in range(3):
            n='(double)n' if scale&2 else 'n'
            expr=f'v[{i}]*{n}' if scale&1 else f'{n}*v[{i}]'
            body+=f'{dest}[{i}]=(float)({expr});'
        body+='return 1;'
        bodies[f'np_{"".join(map(str,squares))}_{length}_{scale}']=body
    return bodies

if __name__=='__main__':
    configs=[[lang,'/O2','/G5'] for lang in ['/TC','/TP']]
    stage=sys.argv[1] if len(sys.argv)>1 else 'initial'
    if stage=='initial':run('promotions-1',promotion_bodies(),configs)
    elif stage=='mixed':run('promotions-mixed',mixed_promotion_bodies(),configs)
    elif stage=='normalize':
        flags=[[lang,'/O2','/Oi',cpu,*extra] for lang,cpu,extra in itertools.product(['/TC','/TP'],['/G5','/G6'],[[],['/Op']])]
        run('normalization-1',normalization_bodies(),flags,target='normalize')
        run('normalization-to-1',normalization_bodies(to=True),flags,target='normalize_to')
    elif stage=='older':
        for mode in ['stock','vc41']:run('promotions-'+mode,promotion_bodies(),configs,mode=mode)
    elif stage=='normpromote':
        pre='#include "D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc/include/math.h"\n'
        flags=[[lang,'/O2','/Oi','/G5'] for lang in ['/TC','/TP']]
        for target in ['normalize','normalize_to']:
            run(target+'-promoted',normalization_promoted_bodies(to=target=='normalize_to'),flags,target=target,preamble=pre)
    else:raise ValueError(stage)
