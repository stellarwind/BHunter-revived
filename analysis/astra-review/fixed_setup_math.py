"""Bounded second matching pass: fixed SP3 C++ /O2 /G5 setup throughout."""
from pathlib import Path
import hashlib,itertools,json,os,subprocess,sys
from targeted_probes import parse_coff

ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent
OUT=HERE/'fixed-math';OUT.mkdir(exist_ok=True)
TOOLS=ROOT/'analysis/toolchain-sp3/bin'
INCLUDE=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc/include')
FLAGS=['/nologo','/c','/ML','/Gy','/TP','/O2','/G5','/I'+str(INCLUDE)]
EXPECTED_SHA='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
EXE=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
assert hashlib.sha256(EXE).hexdigest()==EXPECTED_SHA
TARGETS={'world':(0x401460,45),'cross':(0x4011a0,67),'point':(0x4012e0,116),'direction':(0x401360,107),'local':(0x401490,123)}
TYPES='typedef struct Vec3 {float x,y,z;} Vec3;\ntypedef struct Mat34 {float m[3][4];} Mat34;\n'
PREAMBLE='#include <math.h>\n'+TYPES+'extern "C" {\n'
WORLD=dict(target='world',signature='float x,float y,float z,float *m',body='m[3]=(float)(m[3]+x);m[7]=(float)(y+m[7]);m[11]=(float)(z+m[11]);')

def sha(b):return hashlib.sha256(b).hexdigest()

def compile_cases(label,cases,preamble=PREAMBLE,verbose=True):
    folder=OUT/label;folder.mkdir(exist_ok=True)
    source=preamble+''.join('void __cdecl '+name+'('+case['signature']+'){'+case['body']+'}\n' for name,case in cases.items())+'}\n'
    src=folder/'candidates.cpp';src.write_text(source)
    (folder/'cases.json').write_text(json.dumps(cases,indent=2))
    obj=folder/'candidates.obj'
    env=os.environ.copy();env.pop('CL',None);env.pop('_CL_',None)
    env['PATH']=str(TOOLS)+os.pathsep+env['PATH'];env['INCLUDE']=str(INCLUDE)
    command=[str(TOOLS/'CL.EXE'),*FLAGS,'/Fo'+str(obj),str(src)]
    p=subprocess.run(command,cwd=folder,env=env,capture_output=True,text=True)
    (folder/'compiler.log').write_text(p.stdout+p.stderr)
    assert p.returncode==0,p.stdout+p.stderr
    symbols,sections=parse_coff(obj.read_bytes());results={};unique={}
    for symbol in symbols.values():
        name=symbol['name'].lstrip('_')
        if name not in cases or symbol['section']<=0:continue
        case=cases[name];target=case['target'];va,length=TARGETS[target]
        original=EXE[va-0x400c00:va-0x400c00+length]
        sec=sections[symbol['section']];code=sec['data'][symbol['value']:]
        prefix=next((i for i,(a,b) in enumerate(zip(code,original)) if a!=b),min(len(code),length))
        exact=not sec['relocations'] and code[:length]==original and all(v in (0,0x90,0xcc) for v in code[length:])
        trimmed=code.rstrip(b'\x90\xcc\0');key=sha(target.encode()+trimmed)
        row=dict(name=name,target=target,va=hex(va),target_length=length,object_section_bytes=len(code),code_hex=code.hex(),original_hex=original.hex(),comparison_mask_hex=(b'\xff'*length).hex(),relocations=sec['relocations'],prefix=prefix,exact=exact,unique_key=key)
        results[name]=row
        if key not in unique:unique[key]=dict(target=target,trimmed_code_hex=trimmed.hex(),names=[],exact=exact,prefix=prefix)
        unique[key]['names'].append(name)
    assert len(results)==len(cases),(len(results),len(cases))
    manifest=dict(command=command,cwd=str(folder),flags=FLAGS,original_exe_sha256=EXPECTED_SHA,
                  source_sha256=sha(src.read_bytes()),header_sha256=sha((INCLUDE/'math.h').read_bytes()),object_sha256=sha(obj.read_bytes()),
                  tools={str(TOOLS/f):sha((TOOLS/f).read_bytes()) for f in ['CL.EXE','c1.dll','c1xx.dll','c2.exe']})
    (folder/'manifest.json').write_text(json.dumps(manifest,indent=2))
    (folder/'results.json').write_text(json.dumps(results,indent=2))
    (folder/'unique.json').write_text(json.dumps(unique,indent=2))
    summary={}
    for target in {c['target'] for c in cases.values()}:
        rows=[r for r in results.values() if r['target']==target]
        summary[target]=dict(cases=len(rows),unique=len({r['unique_key'] for r in rows}),matches=[r['name'] for r in rows if r['exact']],best_prefix=max(r['prefix'] for r in rows))
    (folder/'summary.json').write_text(json.dumps(summary,indent=2))
    if verbose:print(label,json.dumps(summary),flush=True)
    return results

def baseline_cases():
    return {
      'bh_vec3_cross':dict(target='cross',signature='const Vec3 *a,const Vec3 *b,Vec3 *out',body='out->x=a->y*b->z-b->y*a->z;out->y=a->z*b->x-b->z*a->x;out->z=b->y*a->x-a->y*b->x;'),
      'bh_mat34_transform_point':dict(target='point',signature='const Mat34 *m,const Vec3 *v,Vec3 *out',body='out->x=v->x*m->m[0][0]+v->z*m->m[0][2]+v->y*m->m[0][1]+m->m[0][3];out->y=m->m[1][0]*v->x+m->m[1][2]*v->z+v->y*m->m[1][1]+m->m[1][3];out->z=m->m[2][0]*v->x+m->m[2][2]*v->z+v->y*m->m[2][1]+m->m[2][3];'),
      'bh_mat34_transform_direction':dict(target='direction',signature='const Mat34 *m,const Vec3 *v,Vec3 *out',body='out->x=v->x*m->m[0][0]+v->y*m->m[0][1]+v->z*m->m[0][2];out->y=m->m[1][0]*v->x+m->m[1][1]*v->y+v->z*m->m[1][2];out->z=m->m[2][0]*v->x+m->m[2][1]*v->y+v->z*m->m[2][2];'),
      'bh_mat34_translate_local':dict(target='local',signature='float x,float y,float z,Mat34 *m',body='m->m[0][3]=x*m->m[0][0]+m->m[0][1]*y+m->m[0][2]*z+m->m[0][3];m->m[1][3]=m->m[1][1]*y+m->m[1][2]*z+m->m[1][0]*x+m->m[1][3];m->m[2][3]=m->m[2][1]*y+m->m[2][2]*z+m->m[2][0]*x+m->m[2][3];'),
      'bh_mat34_translate_world':WORLD,
    }

def cross_cases(promoted=False):
    cases={'positive_control_before':WORLD}
    products=[('a[1]','b[2]'),('b[1]','a[2]'),('a[2]','b[0]'),('b[2]','a[0]'),('b[1]','a[0]'),('a[1]','b[0]')]
    for flip in range(64):
        for promotion in range(64) if promoted else [0]:
            expressions=[]
            for i,(a,b) in enumerate(products):
                if flip>>i&1:a,b=b,a
                if promotion>>i&1:a='(double)'+a
                expressions.append(a+'*'+b)
            for typed,cast in itertools.product([False,True] if not promoted else [False],[False,True] if not promoted else [True]):
                body=''.join('out['+str(i)+']='+('(float)(' if cast else '(')+expressions[2*i]+'-'+expressions[2*i+1]+');' for i in range(3))
                sig='const float *a,const float *b,float *out'
                if typed:
                    for ptr in ['a','b','out']:
                        for i,field in enumerate('xyz'):body=body.replace(ptr+'['+str(i)+']',ptr+'->'+field)
                    sig='const Vec3 *a,const Vec3 *b,Vec3 *out'
                cases[f'cross_f{flip}_p{promotion}_t{int(typed)}_c{int(cast)}']=dict(target='cross',signature=sig,body=body)
    cases['positive_control_after']=WORLD
    return cases

def dot_cases(target):
    cases={'positive_control_before':WORLD}
    for order,group,flip,typed,cast in itertools.product(itertools.permutations(range(3)),['left','right'],range(8),[False,True],[False,True]):
        statements=[]
        for row in range(3):
            products=[]
            for col in range(3):
                v=['x','y','z'][col] if target=='local' else f'v[{col}]'
                m=f'm[{row*4+col}]'
                products.append(f'{m}*{v}' if flip>>col&1 else f'{v}*{m}')
            a,b,c=[products[i] for i in order]
            dot=f'({a}+{b})+{c}' if group=='left' else f'{a}+({b}+{c})'
            if target!='direction':dot=f'({dot})+m[{row*4+3}]'
            if cast:dot=f'(float)({dot})'
            lhs=f'm[{row*4+3}]' if target=='local' else f'out[{row}]'
            statements.append(lhs+'='+dot+';')
        body=''.join(statements)
        sig='float x,float y,float z,float *m' if target=='local' else 'const float *m,const float *v,float *out'
        if typed:
            for i in range(12):body=body.replace(f'm[{i}]',f'm->m[{i//4}][{i%4}]')
            for ptr in ['v','out']:
                for i,field in enumerate('xyz'):body=body.replace(ptr+'['+str(i)+']',ptr+'->'+field)
            sig='float x,float y,float z,Mat34 *m' if target=='local' else 'const Mat34 *m,const Vec3 *v,Vec3 *out'
        name=f'{target}_{"".join(map(str,order))}_{group}_f{flip}_t{int(typed)}_c{int(cast)}'
        cases[name]=dict(target=target,signature=sig,body=body)
    cases['positive_control_after']=WORLD
    return cases

def cross_temporary_cases():
    cases={'positive_control_before':WORLD}
    products=[('a[1]','b[2]'),('b[1]','a[2]'),('a[2]','b[0]'),('b[2]','a[0]'),('b[1]','a[0]'),('a[1]','b[0]')]
    for flip,pt,qt,declorder,subform in itertools.product(range(64),['float','double'],['float','double'],[0,1],range(4)):
        expressions=[]
        for i,(a,b) in enumerate(products):
            if flip>>i&1:a,b=b,a
            expressions.append(a+'*'+b)
        body=''
        for i in range(3):
            pd=f'{pt} p={expressions[2*i]};';qd=f'{qt} q={expressions[2*i+1]};'
            e=['p-q','-(q-p)','p+(-q)','(-q)+p'][subform]
            body+='{'+(pd+qd if declorder==0 else qd+pd)+f'out[{i}]=(float)({e});'+'}'
        cases[f'ct_f{flip}_{pt}_{qt}_{declorder}_{subform}']=dict(target='cross',signature='const float *a,const float *b,float *out',body=body)
    cases['positive_control_after']=WORLD
    return cases

def dot_temporary_cases(target):
    cases={'positive_control_before':WORLD}
    # Capture products or inputs within each row to retain the original
    # sequential-store alias behavior; no values are hoisted across rows.
    for capture,typ,order,group,flip in itertools.product(['products','vector','matrix'],['float','double'],itertools.permutations(range(3)),['left','right'],range(8)):
        body=''
        for row in range(3):
            prefix='';products=[]
            for col in range(3):
                v=['x','y','z'][col] if target=='local' else f'v[{col}]'
                m=f'm[{row*4+col}]'
                if capture=='vector':prefix+=f'{typ} t{col}={v};';v=f't{col}'
                if capture=='matrix':prefix+=f'{typ} t{col}={m};';m=f't{col}'
                p=f'{m}*{v}' if flip>>col&1 else f'{v}*{m}'
                if capture=='products':prefix+=f'{typ} t{col}={p};';p=f't{col}'
                products.append(p)
            a,b,c=[products[i] for i in order]
            dot=f'({a}+{b})+{c}' if group=='left' else f'{a}+({b}+{c})'
            if target!='direction':dot=f'({dot})+m[{row*4+3}]'
            lhs=f'm[{row*4+3}]' if target=='local' else f'out[{row}]'
            body+='{'+prefix+lhs+'=(float)('+dot+');}'
        sig='float x,float y,float z,float *m' if target=='local' else 'const float *m,const float *v,float *out'
        name=f'dt_{capture}_{typ}_{"".join(map(str,order))}_{group}_{flip}'
        cases[name]=dict(target=target,signature=sig,body=body)
    cases['positive_control_after']=WORLD
    return cases

def dot_mixed_rows_cases(target):
    sourcecases=json.loads((OUT/(target+'-temporaries')/'cases.json').read_text())
    sourceresults=json.loads((OUT/(target+'-temporaries')/'results.json').read_text())
    best=max((r for r in sourceresults.values() if r['target']==target),key=lambda r:r['prefix'])
    first=sourcecases[best['name']]['body'].split('}',1)[0]+'}'
    cases={'positive_control_before':WORLD}
    for mode,promote,flip,declorder,group in itertools.product(['captured','direct'],range(8),range(8),itertools.permutations(range(3)),['left','right']):
        body=first
        for row in [1,2]:
            prefix='';products=[]
            if mode=='captured':
                for col in declorder:prefix+=('double' if promote>>col&1 else 'float')+f' t{col}=v[{col}];'
            for col in range(3):
                v=f't{col}' if mode=='captured' else (f'(double)v[{col}]' if promote>>col&1 else f'v[{col}]')
                m=f'm[{row*4+col}]'
                products.append(f'{m}*{v}' if flip>>col&1 else f'{v}*{m}')
            order=[0,1,2] if target=='direction' else [0,2,1]
            a,b,c=[products[col] for col in order]
            dot=f'{a}+({b}+{c})' if group=='right' else f'({a}+{b})+{c}'
            if target=='point':dot=f'({dot})+m[{row*4+3}]'
            body+='{'+prefix+f'out[{row}]=(float)({dot});'+'}'
        cases[f'mixed_{mode}_{promote}_{flip}_{"".join(map(str,declorder))}_{group}']=dict(target=target,signature='const float *m,const float *v,float *out',body=body)
    cases['positive_control_after']=WORLD
    return cases

def dot_promoted_rows_cases(target):
    sourcecases=json.loads((OUT/(target+'-temporaries')/'cases.json').read_text())
    sourceresults=json.loads((OUT/(target+'-temporaries')/'results.json').read_text())
    best=max((r for r in sourceresults.values() if r['target']==target),key=lambda r:r['prefix'])
    first=sourcecases[best['name']]['body'].split('}',1)[0]+'}'
    cases={'positive_control_before':WORLD}
    for promote,flip in itertools.product(itertools.product(range(4),repeat=3),range(8)):
        body=first
        for row in [1,2]:
            products=[]
            for col,p in enumerate(promote):
                v=('(double)' if p&1 else '')+f'v[{col}]'
                m=('(double)' if p&2 else '')+f'm[{row*4+col}]'
                products.append(f'{m}*{v}' if flip>>col&1 else f'{v}*{m}')
            order=[0,1,2] if target=='direction' else [0,2,1]
            a,b,c=[products[col] for col in order]
            dot=f'{a}+({b}+{c})'
            if target=='point':dot=f'({dot})+m[{row*4+3}]'
            body+='{'+f'out[{row}]=(float)({dot});'+'}'
        cases[f'pr_{"".join(map(str,promote))}_{flip}']=dict(target=target,signature='const float *m,const float *v,float *out',body=body)
    cases['positive_control_after']=WORLD
    return cases

def cross_input_cases():
    inputs=[['a[1]','b[2]','b[1]','a[2]'],['a[2]','b[0]','b[2]','a[0]'],['b[1]','a[0]','a[1]','b[0]']]
    cases={'positive_control_before':WORLD}
    for types,order,flip in itertools.product(range(16),itertools.permutations(range(4)),range(4)):
        body=''
        for row,vars in enumerate(inputs):
            body+='{' + ''.join(('double' if types>>i&1 else 'float')+f' t{i}={vars[i]};' for i in order)
            p='t1*t0' if flip&1 else 't0*t1';q='t3*t2' if flip&2 else 't2*t3'
            body+=f'out[{row}]=(float)({p}-{q});'+'}'
        cases[f'ci_{types}_{"".join(map(str,order))}_{flip}']=dict(target='cross',signature='const float *a,const float *b,float *out',body=body)
    cases['positive_control_after']=WORLD
    return cases

def dot_capture_order_cases(target):
    sources=json.loads((OUT/(target+'-temporaries')/'cases.json').read_text())
    results=json.loads((OUT/(target+'-temporaries')/'results.json').read_text())
    best=max((r for r in results.values() if r['target']==target),key=lambda r:r['prefix'])
    first=sources[best['name']]['body'].split('}',1)[0]+'}'
    cases={'positive_control_before':WORLD}
    for order,flip,terms,group in itertools.product(itertools.permutations(range(4)),range(8),itertools.permutations(range(3)),['left','right']):
        body=first
        for row in [1,2]:
            col=2 if target=='direction' else 1;other=3-col
            vals=[f'm[{row*4+col}]',f'v[{col}]',f'm[{row*4+other}]',f'm[{row*4}]']
            decl=''.join(f'float t{i}={vals[i]};' for i in order)
            pairs=[('t3','v[0]'),('t2',f'v[{other}]'),('t0','t1')]
            products=[b+'*'+a if flip>>i&1 else a+'*'+b for i,(a,b) in enumerate(pairs)]
            a,b,c=[products[i] for i in terms]
            expr=f'{a}+({b}+{c})' if group=='right' else f'({a}+{b})+{c}'
            if target=='point':expr=f'({expr})+m[{row*4+3}]'
            body+='{'+decl+f'out[{row}]=(float)({expr});'+'}'
        cases[f'co_{"".join(map(str,order))}_{flip}_{"".join(map(str,terms))}_{group}']=dict(target=target,signature='const float *m,const float *v,float *out',body=body)
    cases['positive_control_after']=WORLD
    return cases

if __name__=='__main__':
    stage=sys.argv[1] if len(sys.argv)>1 else 'baseline'
    if stage=='baseline':
        cases=baseline_cases();compile_cases('baseline-combined',cases)
        for name,case in cases.items():compile_cases('baseline-single-'+case['target'],{name:case})
    elif stage=='expressions':
        compile_cases('cross-expressions',cross_cases())
        for target in ['direction','point','local']:compile_cases(target+'-expressions',dot_cases(target))
    elif stage=='cross-promoted':compile_cases('cross-promoted',cross_cases(promoted=True))
    elif stage=='temporaries':
        compile_cases('cross-temporaries',cross_temporary_cases())
        for target in ['direction','point','local']:compile_cases(target+'-temporaries',dot_temporary_cases(target))
    elif stage=='mixed-rows':
        for target in ['direction','point']:compile_cases(target+'-mixed-rows',dot_mixed_rows_cases(target))
    elif stage=='refinement':
        compile_cases('cross-inputs',cross_input_cases())
        for target in ['direction','point']:compile_cases(target+'-promoted-rows',dot_promoted_rows_cases(target))
    elif stage=='capture-order':
        for target in ['point','direction']:compile_cases(target+'-capture-order',dot_capture_order_cases(target))
    else:raise ValueError(stage)
