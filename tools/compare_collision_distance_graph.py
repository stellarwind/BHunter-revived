"""Read actual original and compiled polygon-plane expression DAGs, including spills."""
import argparse
import json
import re
import struct
from pathlib import Path
from compare_ai_damage_query import digest, image_bytes
from compare_collision_cut_graph import op, f32
from prove_collision_iteration import read_owner
import prove_collision_iteration as engine

ROOT=Path(__file__).resolve().parents[1]


def run(rows, registers, memory, calls=None, owners=None, initial_fp=None):
    lookup={r['address']:r for r in rows};pc=rows[0]['address'];fp=list(initial_fp or []);zero=False;covered=[]
    def address(arg):
        value=0
        for term in re.search(r'\[([^]]+)\]',arg)[1].upper().replace(' ','').replace('-','+-').split('+'):
            product=1
            for part in term.split('*'):
                if part:product*=registers[part] if part in registers else int(part,0)
            value+=product
        return value
    def read(arg):
        st=re.fullmatch(r'ST\(?([0-7])?\)?',arg)
        if st:return fp[int(st[1] or 0)]
        if '[' in arg:return memory[address(arg)]
        return registers[arg] if arg in registers else int(arg,0)
    def write(arg,value):
        if '[' in arg:memory[address(arg)]=value
        else:registers[arg]=value
    for _ in range(200):
        if pc not in lookup:return fp,covered
        row=lookup[pc];m=row['mnemonic'];arg=row['operands'].upper();parts=[p.strip() for p in arg.split(',')]
        next_pc=pc+len(row['data']);covered.append(pc)
        if m=='MOV':write(parts[0],read(parts[1]))
        elif m=='LEA':write(parts[0],address(parts[1]))
        elif m=='PUSH':v=read(arg);registers['ESP']-=4;memory[registers['ESP']]=v
        elif m in ('ADD','SUB'):registers[parts[0]]+=read(parts[1])*(1 if m=='ADD' else -1)
        elif m=='TEST':zero=(read(parts[0])&read(parts[1]))==0
        elif m in ('JE','JZ','JNE','JNZ','JMP'):
            if m=='JMP' or (not zero if m in ('JNE','JNZ') else zero):next_pc=int(arg,0)
        elif m=='RET':return fp,covered
        elif m=='CALL':
            assert not fp
            target=calls[pc];body,child_calls=owners[target]
            child=dict(registers);child['ESP']-=4
            fp,_=run(body,child,memory,child_calls,owners)
            for r in ('EAX','ECX','EDX'):
                if r in child:registers[r]=child[r]
            assert len(fp)==(0 if target=='_bh_round_float' else 1)
        elif m=='FLD':fp.insert(0,read(arg))
        elif m=='FXCH':i=row['data'][1]&7;fp[0],fp[i]=fp[i],fp[0]
        elif m in ('FMUL','FADD'):
            i=0 if len(parts)==1 else int(re.fullmatch(r'ST\(?([0-7])?\)?',parts[0])[1] or 0)
            fp[i]=op('mul' if m=='FMUL' else 'add',fp[i],read(parts[-1]))
        elif m in ('FMULP','FADDP'):
            i=row['data'][1]&7;fp[i]=op('mul' if m=='FMULP' else 'add',fp[i],fp[0]);fp.pop(0)
        elif m in ('FST','FSTP'):
            value=fp[0] if 'QWORD' in arg or 'DOUBLE' in arg else f32(fp[0]);write(arg,value)
            if m=='FSTP':fp.pop(0)
        else:raise AssertionError((pc,m,arg))
        pc=next_pc
    raise AssertionError('Unbounded expression slice')


def evaluate(graph, values):
    if graph[0]=='input32':return values[graph[1]]
    if graph[0]=='f32':return struct.unpack('<f',struct.pack('<f',evaluate(graph[1],values)))[0]
    a,b=evaluate(graph[1],values),evaluate(graph[2],values)
    return a+b if graph[0]=='add53' else a*b


def caller_proofs(obj, output):
    """Verify both actual face_cut call sites forward the owning variant."""
    z,B,State,walk,prove=engine.z,engine.B,engine.State,engine.walk,engine.prove
    rows,calls,_,_=read_owner(obj,'_face_cut')
    actual=sorted(at for at,name in calls.items() if name=='_cut_distance')
    assert actual==[0x9b,0x12a]
    engine.DUMP_SMT=output.parent/(output.stem+'-caller-smt')
    engine.DUMP_SMT.mkdir(parents=True,exist_ok=True)
    sp,frame,n=0x10000,0x10200,0x20000
    first,cached,plane=z.BitVecs('distance.first distance.cached distance.plane-bits',32)
    for seed,start,stop in ((True,0x6e,actual[0]),(False,0x115,actual[1])):
        s=State('distance-caller.'+str(seed),start)
        s.path=[z.Or(first==0,first==1),z.Or(cached==0,cached==1)]
        s.reg.update(ESP=B(sp),EBP=B(frame))
        for at,value in ((frame+0x14,n),(frame+0x18,plane),(frame+0x1c,first),(frame+0x24,cached)):
            s.store(at,value)
        words=[z.BitVec('distance.point-bits.'+str(i),32) for i in range(3)]
        for i,v in enumerate(words):s.store(sp+0x54+4*i,v)
        for st in walk(rows,calls,s,{stop},lambda *_:(_ for _ in ()).throw(AssertionError('Unexpected plane-call prefix dependency'))):
            ptr=st.reg['ESP'];point=sp+(0x48 if seed else 0x3c)
            prove('distance-caller.arguments.'+str(seed),st.path,
                  z.And(*[st.load(ptr+4*i)==v for i,v in enumerate([B(n),B(point),plane,first,cached])],
                        *([st.load(point+4*i)==v for i,v in enumerate(words)] if seed else [])))
    return engine.PROOFS


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--object',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--round-object',type=Path)
    parser.add_argument('--expect-mismatch',action='store_true')
    args=parser.parse_args()
    image=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert digest(image)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    normal=[('input32','normal.'+str(i)) for i in range(3)]
    point=[('input32','point.'+str(i)) for i in range(3)]
    plane=('input32','plane')
    sp,node,p=0x10000,0x20000,0x30000
    owners={}
    for name in ('_cut_distance','_face_dot'):
        body,calls,_,_=read_owner(args.object,name);owners[name]=(body,calls)
    cut_rows,cut_calls,_,_=read_owner(args.object,'_face_cut')
    post_seed=[]
    if not args.expect_mismatch:
        helper,helper_calls,_,_=read_owner(args.round_object or args.object.parent/'bh_round.obj','_bh_round_float')
        assert b''.join(r['data'] for r in helper).hex()=='dd4424088b442404d918c3'
        owners['_bh_round_float']=(helper,helper_calls)
        call=next(at for at,name in cut_calls.items() if name=='_cut_distance')
        rounding=next(at for at,name in cut_calls.items() if name=='_bh_round_float' and at>call)
        post_seed=[r for r in cut_rows if call+5<=r['address']<rounding+5]
    cache={}
    cases=[]
    specs=[(False,True,True,0x44c3f1,0x44c421),(False,True,False,0x44c45e,0x44c48d),
           (True,True,True,0x44cca6,0x44ccd3),(True,True,False,0x44cd13,0x44cd38),
           (False,False,True,0x44c706,0x44c73c),(False,False,False,0x44c768,0x44c797),
           (True,False,True,0x44cf3d,0x44cf67),(True,False,False,0x44cf8f,0x44cfb5)]
    for cached,first,seed,start,stop in specs:
        path='META/m4/face-contact/original/disassembly.json' if cached else 'META/m4/collision-traversal/original/0044c200.disassembly.json'
        if path not in cache:cache[path]=json.loads((ROOT/path).read_text())
        rows=[dict(address=int(r['address'],16),data=bytes.fromhex(r['bytes']),mnemonic=r['mnemonic'],operands=', '.join(r['operands']))
              for r in cache[path] if start<=int(r['address'],16)<stop]
        assert b''.join(r['data'] for r in rows)==image_bytes(image,start,stop-start)
        memory={node+24+4*i:normal[i] for i in range(3)};memory[node+36]=plane
        memory.update({p+4*i:point[i] for i in range(3)})
        registers=dict(ESP=sp,EBX=node,EDI=node,ESI=p,ECX=p,EDX=p,EAX=0,EBP=0)
        if first:
            offset=(0x64 if seed else 0x58) if cached else (0x6c if seed else 0x78)
            memory.update({sp+offset+4*i:point[i] for i in range(3)})
            if seed:registers['ESP']=sp-12
            elif cached:registers['EDX']=node+24
            else:registers['ESP']=sp-12;memory[sp+0x2c]=node+24
            destination=sp+(0x14 if cached else 0x10)
        else:
            offset=0x70 if cached else 0x84
            memory.update({sp+offset+4*i:normal[i] for i in range(3)})
            memory[sp+(0x2c if cached else 0x18)]=plane
            destination=sp+(0x14 if cached else 0x10)
        fp,covered=run(rows,registers,memory)
        a=memory[destination] if seed else fp[0]
        assert len(fp)==(0 if seed else 1)
        # Run the actual compiled helper with its original calling convention.
        memory={sp+4:node,sp+8:p,sp+12:plane,sp+16:int(first),sp+20:int(cached)}
        memory.update({node+4*i:normal[i] for i in range(3)})
        memory.update({p+4*i:point[i] for i in range(3)})
        fp,source_covered=run(*[owners['_cut_distance'][0],dict(ESP=sp),memory],owners['_cut_distance'][1],owners)
        assert len(fp)==1
        b=fp[0]
        if seed:
            if args.expect_mismatch:b=f32(b)
            else:
                # Actual caller's binary64 return spill and actual11B rounding
                # helper, rather than a source-derived presumed float store.
                spill_memory={}
                spilled,_=run(post_seed,dict(ESP=sp-20),spill_memory,cut_calls,owners,[b])
                assert not spilled
                b=spill_memory[sp+0x10]
        n=struct.unpack('<f',struct.pack('<f',.57735026))[0]
        values={'normal.'+str(i):n for i in range(3)}
        values.update({'point.0':2.**54,'point.1':-2.**54,'point.2':1.,'plane':0.})
        cases.append(dict(owner='0044cc20' if cached else '0044c200',first=first,seed=seed,
                          original_slice=[hex(start),hex(stop)],original_expression=a,source_expression=b,
                          matches=a==b,covered=covered,source_covered=source_covered,
                          cancellation_probe=dict(inputs=values,original=evaluate(a,values).hex(),source=evaluate(b,values).hex())))
    mismatches=sum(not c['matches'] for c in cases)
    forwarding=[] if args.expect_mismatch else caller_proofs(args.object,args.output)
    result=dict(passed=(mismatches>0 if args.expect_mismatch else mismatches==0),static_only=True,
                original_sha256=digest(image),object=str(args.object),object_sha256=digest(args.object.read_bytes()),
                mismatches=mismatches,cases=cases,caller_obligations=forwarding,candidate_read=False,
                scope='Actual original/compiled PC53 finite expression DAGs; binary32 seed spills explicit. Individual commutation only, no reassociation. Cancellation probe evaluates decoded DAGs at binary64 precision; it does not execute original code.')
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2)+'\n')
    assert result['passed'],(mismatches,'Actual compiled plane expressions differ')
    print(str(len(cases))+' original/compiled polygon-plane graphs: '+str(mismatches)+' discrepancies')


if __name__=='__main__':main()
