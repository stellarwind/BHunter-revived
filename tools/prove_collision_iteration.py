"""Static SMT refinement of the actual production collision continuation machine.

Reads current COFF and immutable original instructions. Does not execute code,
run integration tests, or read the uninstalled recursive candidate.
"""
import copy
import argparse
import json
import re
import sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'build/proof-deps'))
import z3 as z
from compare_ai_damage_query import owner_instructions,parse_coff,image_bytes,digest

OUT=ROOT/'META/m4/playable/collision-iteration'
B=lambda n:z.BitVecVal(n&0xffffffff,32)
Q,G,A,NODES,STACK,SUPP,LEAVES,FACES=0x100000,0x110000,0x120000,0x1000000,0x2000000,0x3000000,0x4000000,0x5000000
SP=0x10000
PROOFS=[]
FORWARDING_QUERIES=0
DUMP_SMT=None


def simplify(v):return z.simplify(v)
def concrete(v):return simplify(v).as_long()
def feasible(conditions):
    solver=z.Solver();solver.set(timeout=15000);solver.add(*conditions)
    result=solver.check();assert result!=z.unknown,solver.reason_unknown()
    return result==z.sat


def prove(name,assumptions,claim):
    assert feasible(assumptions),(name,'Vacuous transition assumptions')
    solver=z.Solver();solver.set(timeout=15000);solver.add(*assumptions,z.Not(claim))
    result=solver.check()
    assert result==z.unsat,(name,result,str(solver.model()) if result==z.sat else solver.reason_unknown())
    record=dict(name=name,result='unsat',nonvacuous=True,meaning='No counterexample to this transition under its declared assumptions')
    if DUMP_SMT is not None:
        filename=f'{len(PROOFS):03d}-'+re.sub(r'[^a-zA-Z0-9_.-]','_',name)+'.smt2'
        query=solver.to_smt2();(DUMP_SMT/filename).write_text(query)
        record.update(smt_file='smt/'+filename,smt_sha256=digest((DUMP_SMT/filename).read_bytes()))
    PROOFS.append(record)


class State:
    def __init__(self,name,pc=0):
        self.pc=pc;self.steps=0;self.path=[];self.events=[];self.covered=set()
        self.reg={r:z.BitVec(name+'.'+r,32) for r in ('EAX','EBX','ECX','EDX','ESI','EDI','EBP','ESP')}
        self.mem=z.Array(name+'.memory',z.BitVecSort(32),z.BitVecSort(32))
        self.cells=[]
        self.zero=z.BoolVal(False);self.carry=z.BoolVal(False);self.sign=z.BoolVal(False);self.overflow=z.BoolVal(False)
        self.returned=False
    def clone(self):
        s=copy.copy(self);s.reg=dict(self.reg);s.path=list(self.path);s.events=list(self.events);s.covered=set(self.covered);s.cells=list(self.cells);return s
    def load(self,addr):
        global FORWARDING_QUERIES
        addr=simplify(B(addr) if isinstance(addr,int) else addr)
        # Semantics-preserving store forwarding: every non-syntactic alias
        # decision is discharged against the current path, never guessed from
        # a convenient fixed address or ignored pointer operand.
        for stored,value in reversed(self.cells):
            difference=simplify(addr-stored)
            if z.is_bv_value(difference):
                if difference.as_long()==0:return value
                continue
            FORWARDING_QUERIES+=1
            if not feasible(self.path+[addr!=stored]):return value
            if feasible(self.path+[addr==stored]):break
        return simplify(z.Select(self.mem,addr))
    def store(self,addr,value):
        addr=simplify(B(addr) if isinstance(addr,int) else addr);value=simplify(B(value) if isinstance(value,int) else value)
        self.mem=z.Store(self.mem,addr,value);self.cells.append((addr,value))
    def address(self,operand):
        inner=re.search(r'\[([^]]+)\]',operand)[1].upper().replace(' ','').replace('-','+-')
        value=B(0)
        for term in inner.split('+'):
            if not term:continue
            factors=term.split('*');product=B(1)
            for f in factors:product=product*(self.reg[f] if f in self.reg else B(int(f,0)))
            value=value+product
        return simplify(value)
    def read(self,operand):
        operand=operand.strip().upper()
        if operand in self.reg:return self.reg[operand]
        if '[' in operand:return self.load(self.address(operand))
        return B(int(operand,0))
    def write(self,operand,value):
        operand=operand.strip().upper();value=simplify(value)
        if operand in self.reg:self.reg[operand]=value
        else:self.store(self.address(operand),value)
    def compare(self,a,b):
        v=a-b;self.zero=v==0;self.carry=z.ULT(a,b);self.sign=v<0
        self.overflow=z.And((a<0)!=(b<0),(v<0)!=(a<0))


def walk(rows,calls,initial,stops,hook,limit=300):
    lookup={r['address']:r for r in rows};pending=[initial];finished=[]
    while pending:
        s=pending.pop()
        while s.steps<limit:
            if s.steps and (s.pc in stops or s.returned):finished.append(s);break
            r=lookup[s.pc];m=r['mnemonic'];arg=r['operands'].upper();p=[v.strip() for v in arg.split(',')]
            s.covered.add(s.pc);s.steps+=1;next_=s.pc+len(r['data'])
            if m=='MOV':s.write(p[0],s.read(p[1]))
            elif m=='LEA':s.write(p[0],s.address(p[1]))
            elif m=='PUSH':
                v=s.read(arg);s.reg['ESP']=simplify(s.reg['ESP']-4);s.store(s.reg['ESP'],v)
            elif m=='POP':
                v=s.load(s.reg['ESP']);s.reg['ESP']=simplify(s.reg['ESP']+4);s.write(arg,v)
            elif m in ('ADD','SUB','INC','DEC'):
                a=s.read(p[0]);b=s.read(p[1]) if len(p)>1 else B(1)
                v=a+b if m in ('ADD','INC') else a-b;s.write(p[0],v)
                s.zero=v==0;s.sign=v<0
            elif m in ('XOR','OR','AND'):
                a,b=s.read(p[0]),s.read(p[1]);v=a^b if m=='XOR' else a|b if m=='OR' else a&b
                s.write(p[0],v);s.zero=v==0;s.sign=v<0;s.overflow=z.BoolVal(False);s.carry=z.BoolVal(False)
            elif m in ('SHL','SHR'):
                a,b=s.read(p[0]),concrete(s.read(p[1]));s.write(p[0],a<<b if m=='SHL' else z.LShR(a,b))
            elif m=='CMP':s.compare(s.read(p[0]),s.read(p[1]))
            elif m=='TEST':
                v=s.read(p[0])&s.read(p[1]);s.zero=v==0;s.sign=v<0;s.overflow=z.BoolVal(False);s.carry=z.BoolVal(False)
            elif m.startswith('J'):
                condition={'JMP':z.BoolVal(True),'JE':s.zero,'JZ':s.zero,'JNE':z.Not(s.zero),'JNZ':z.Not(s.zero),
                    'JL':s.sign!=s.overflow,'JGE':s.sign==s.overflow,'JG':z.And(z.Not(s.zero),s.sign==s.overflow),
                    'JLE':z.Or(s.zero,s.sign!=s.overflow),'JB':s.carry,'JAE':z.Not(s.carry),
                    'JA':z.And(z.Not(s.carry),z.Not(s.zero)),'JBE':z.Or(s.carry,s.zero)}[m]
                target=int(arg,0);yes=z.simplify(condition);no=z.simplify(z.Not(condition))
                if feasible(s.path+[yes]):
                    branch=s.clone();branch.path.append(yes);branch.pc=target;pending.append(branch)
                if not feasible(s.path+[no]):break
                s.path.append(no)
            elif m=='CALL':hook(s,calls[s.pc])
            elif m=='REP':
                count=s.reg['ECX'];destination=s.reg['EDI'];before=s.mem
                if arg.startswith('STOSD'):
                    address=z.BitVec('rep.address.'+str(s.pc),32)
                    s.mem=z.Lambda(address,z.If(z.And(z.UGE(address,destination),z.ULT(address,destination+4*count),
                                                    ((address-destination)&3)==0),s.reg['EAX'],z.Select(before,address)))
                    s.cells=[]
                    s.reg['EDI']=simplify(destination+4*count);s.reg['ECX']=B(0)
                elif arg.startswith('STOSB'):
                    prove('cleanup.byte-remainder-is-zero',s.path,count==0);s.reg['ECX']=B(0)
                else:
                    assert arg.startswith('MOVSD') and concrete(count)==14
                    for index in range(14):s.store(destination+4*index,s.load(s.reg['ESI']+4*index))
                    s.reg['ESI']=simplify(s.reg['ESI']+56);s.reg['EDI']=simplify(destination+56);s.reg['ECX']=B(0)
            elif m=='RET':s.returned=True
            else:raise AssertionError((hex(s.pc),m,arg))
            s.pc=next_
        else:raise AssertionError(('Unbounded local transition',hex(s.pc)))
    assert finished
    return finished


def read_owner(path,name):
    symbols,sections=parse_coff(path.read_bytes());symbol=next(s for s in symbols.values() if s['name']==name)
    rows,listing,end,_=owner_instructions(path,symbols,sections,name)
    calls={}
    for rel in sections[symbol['section']]['relocations']:
        row=next(r for r in rows if r['address']<=rel['offset']<r['address']+len(r['data']))
        if rel['kind']==20:calls[row['address']]=rel['symbol']['name']
    return rows,calls,listing,end


def gate_graph(rows,start,stop,registers,memory):
    """Exact PC53 x87 expression DAG; no reassociation or approximate values."""
    lookup={r['address']:r for r in rows};fp=[];pc=start;comparison=None;zero=False;covered=[]
    def address(text):
        result=0
        for term in re.search(r'\[([^]]+)\]',text)[1].upper().replace(' ','').split('+'):
            product=1
            for f in term.split('*'):product*=registers[f] if f in registers else int(f,0)
            result+=product
        return result
    def read(text):
        text=text.upper();st=re.fullmatch(r'ST\(?([0-7])?\)?',text)
        if st:return fp[int(st[1] or 0)]
        if '[' in text:return memory[address(text)]
        return registers[text] if text in registers else int(text,0)
    def op(name,left,right):
        if name in ('add','mul') and repr(left)>repr(right):left,right=right,left
        return (name+'53',left,right)
    for _ in range(100):
        if pc==stop:
            assert comparison is not None and not fp
            return comparison,covered
        row=lookup[pc];m=row['mnemonic'];arg=row['operands'].upper();p=[v.strip() for v in arg.split(',')]
        next_=pc+len(row['data']);covered.append(pc)
        if m=='MOV':registers[p[0]]=read(p[1])
        elif m=='LEA':registers[p[0]]=address(p[1])
        elif m=='ADD':registers[p[0]]+=read(p[1])
        elif m=='TEST':
            if p[0]=='AH':assert p[1]=='0X1' and comparison is not None
            else:zero=read(p[0])&read(p[1])==0
        elif m in ('JE','JZ','JMP'):
            if m=='JMP' or zero:next_=int(arg,0)
        elif m=='FLD':fp.insert(0,read(arg))
        elif m=='FXCH':
            index=(row['data'][1]&7);fp[0],fp[index]=fp[index],fp[0]
        elif m in ('FADD','FSUB','FSUBR','FMUL'):
            index=0;left,right=fp[0],read(arg) if len(p)==1 else read(p[1])
            if len(p)==2:
                index=int(re.fullmatch(r'ST\(?([0-7])?\)?',p[0])[1] or 0);left=fp[index]
            if m=='FSUBR':left,right=right,left
            fp[index]=op({'FADD':'add','FSUB':'sub','FSUBR':'sub','FMUL':'mul'}[m],left,right)
        elif m in ('FADDP','FSUBP','FSUBRP'):
            index=row['data'][1]&7;left,right=fp[index],fp[0]
            if m=='FSUBRP':left,right=right,left
            fp[index]=op('add' if m=='FADDP' else 'sub',left,right);fp.pop(0)
        elif m=='FCOMPP':comparison=(fp[0],fp[1]);fp.pop(0);fp.pop(0)
        elif m=='FSTP':
            assert arg in ('ST0','ST(0)');fp.pop(0)
        elif m=='FNSTSW':assert arg=='AX' and comparison is not None
        else:raise AssertionError((hex(pc),m,arg))
        pc=next_
    raise AssertionError('Unbounded gate graph')


def main():
    global DUMP_SMT
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--object',type=Path,default=ROOT/'build/game/bh_collision_query.obj')
    parser.add_argument('--native-object',type=Path)
    parser.add_argument('--dump-smt',action='store_true')
    parser.add_argument('--output',type=Path,default=OUT)
    args=parser.parse_args();outdir=args.output;outdir.mkdir(parents=True,exist_ok=True)
    if args.dump_smt:DUMP_SMT=outdir/'smt';DUMP_SMT.mkdir(exist_ok=True)
    obj=args.object
    native_equivalence=None
    if args.native_object:
        first,fs=parse_coff(obj.read_bytes());second,ss=parse_coff(args.native_object.read_bytes())
        left={s['name']:s for s in first.values() if s['section']>0 and s['type']==32}
        right={s['name']:s for s in second.values() if s['section']>0 and s['type']==32}
        assert left.keys()==right.keys()
        checked=[]
        for name,a in left.items():
            b=right[name];x,y=fs[a['section']],ss[b['section']]
            assert x['data']==y['data'],('Native/production function variant differs',name)
            def relocations(section,sections):
                result=[]
                for r in section['relocations']:
                    s=r['symbol'];target=sections[s['section']]['data'] if s['section']>0 and s['type']!=32 else None
                    result.append((r['offset'],r['kind'],s['name'],s['value'],target))
                return result
            assert relocations(x,fs)==relocations(y,ss),('Native/production bindings differ',name)
            checked.append(name)
        native_equivalence=dict(passed=True,native_object=str(args.native_object),native_object_sha256=digest(args.native_object.read_bytes()),
                                complete_functions_and_referenced_data=sorted(checked))
    execute,ecalls,elisting,elen=read_owner(obj,'_execute')
    mark,mcalls,mlisting,mlen=read_owner(obj,'_mark_faces')
    leaf,lcalls,llisting,llen=read_owner(obj,'_leaf')
    wrapper,wcalls,wlisting,wlen=read_owner(obj,'_bh_collision_query_run')
    step,scalls,slisting,slen=read_owner(obj,'_step')
    spheres,gcalls,glisting,glen=read_owner(obj,'_spheres')
    image=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert digest(image)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    original=[];ocalls={}
    for row in json.loads((ROOT/'META/m4/collision-traversal/original/0044d2f0.disassembly.json').read_text()):
        va=int(row['address'],16);data=bytes.fromhex(row['bytes']);assert data==image_bytes(image,va,len(data))
        operand=', '.join(row['operands'])
        if row['mnemonic'].startswith('J'):operand=hex(int(operand,16)-0x44d2f0)
        if row['mnemonic']=='CALL':ocalls[va-0x44d2f0]=int(row['operands'][0],16)
        original.append(dict(address=va-0x44d2f0,data=data,mnemonic=row['mnemonic'],operands=operand))
    assert b''.join(r['data'] for r in original)==image_bytes(image,0x44d2f0,391)
    # Direct original-vs-production gate arithmetic proof. Matrix output is an
    # explicit float32 dependency; original/source logical bounds fields bind
    # to the same selected center/radius values, despite different owner views.
    point=[('input32','transformed.'+str(i)) for i in range(3)]
    center=[('input32','destination-center.'+str(i)) for i in range(3)]
    ar,br=('input32','source-radius'),('input32','destination-radius')
    om={0x4e0fd0:0x130100,0x4e0fcc:0x130000,0x130100+0x6c:0x900000,
        0x130100+0x70:0x900100,0x130000+0x70:0x900200}
    for i in range(3):om[SP+12+4*i]=point[i];om[0x900000+12*3+4*i]=center[i]
    om[0x900100+4*3]=br;om[0x900200+4*2]=ar
    old_gate,old_gate_covered=gate_graph(original,0x49,0xa5,dict(ESP=SP-12,EBX=3,ESI=2),om)
    sm={0x120100+0x30:0x900300,0x120000+0x30:0x900400,SP+0x3c:1}
    for i in range(3):sm[SP+16+4*i]=point[i];sm[0x900300+20*3+4*i]=center[i]
    sm[0x900300+20*3+16]=br;sm[0x900400+20*2+16]=ar
    new_gate,new_gate_covered=gate_graph(spheres,0x15d,0x1cb,dict(ESP=SP,EBX=0x120100,EBP=0x120000,ESI=20*3,EDI=20*2),sm)
    assert old_gate==new_gate,('Production sphere gate expression differs',old_gate,new_gate)
    assert next(r for r in original if r['address']==0xa5)['mnemonic']=='JNZ'
    assert next(r for r in spheres if r['address']==0x1cb)['mnemonic']=='JNE'
    gate=dict(passed=True,original_comparison=old_gate,production_comparison=new_gate,
              original_covered=old_gate_covered,production_covered=new_gate_covered,
              domain='Finite float32 selected inputs; PC53 nearest masked; source validation copies finite and radius nonnegative. Individual add/mul commute; no reassociation.',
              dependency='Same logical node center/radius values and transformed float32 point; owner field views and matrix transform remain declared dependencies')
    d,n,bn,front,back,capacity=z.BitVecs('depth node destination front back capacity',32)
    transform,world,out=B(0x900000),B(0x900100),B(0x900200)
    base=[z.UGE(d,1),z.ULE(d,65535),z.UGE(n,0),z.ULE(n,65534),z.UGE(bn,0),z.ULE(bn,65534),
          z.UGE(capacity,d+1),z.ULE(capacity,65536),z.Or(front==-1,z.And(front>=0,z.ULE(front,65534))),
          z.Or(back==-1,z.And(back>=0,z.ULE(back,65534)))]
    # Verify the actual logical-center and matrix arguments for arbitrary node
    # indices, independently of the numerical post-transform expression proof.
    original_centers,source_bounds=0xa000000,0xb000000
    s=State('original-transform-entry');s.reg['ESP']=B(SP);s.path=list(base)
    s.store(SP+4,n);s.store(SP+8,bn);s.store(0x4e0fcc,0x130000)
    s.store(0x130000+0x5c,NODES);s.store(0x130000+0x6c,original_centers)
    for st in walk(original,ocalls,s,{0x44},lambda *_:(_ for _ in ()).throw(AssertionError('Unexpected original prefix call'))):
        prove('original-transform-logical-center',st.path,z.And(st.load(st.reg['ESP'])==0x4e1010,
              st.load(st.reg['ESP']+4)==B(original_centers)+12*n,st.load(st.reg['ESP']+8)==SP-12,
              st.reg['EDI']==B(NODES)+56*n))
    s=State('production-transform-entry');s.reg['ESP']=B(SP);s.path=list(base)
    for at,value in ((SP+4,Q),(SP+8,n),(SP+12,bn),(SP+16,transform),(SP+20,1),
                     (Q,G),(Q+4,G+0x100),(G,A),(G+0x100,A+0x100),(A+0x30,source_bounds)):
        s.store(at,value)
    valid_calls=[]
    def valid_node_cut(st,callee):
        assert callee=='_valid_node';ordinal=len(valid_calls);valid_calls.append(ordinal)
        prove('valid-node-arguments.'+str(ordinal),st.path,z.And(st.load(st.reg['ESP'])==(A if ordinal==0 else A+0x100),
              st.load(st.reg['ESP']+4)==(n if ordinal==0 else bn)))
        st.reg.update(EAX=B(1),ECX=B(0xdddd),EDX=B(0xeeee))
    for st in walk(spheres,gcalls,s,{0x58},valid_node_cut):
        prove('production-transform-logical-center',st.path,z.And(st.load(st.reg['ESP'])==transform,
              st.load(st.reg['ESP']+4)==B(source_bounds)+20*n,st.load(st.reg['ESP']+8)==SP-24,
              st.reg['EDI']==20*n,st.reg['ESI']==bn,st.reg['EBP']==A,st.reg['EBX']==A+0x100))
    assert len(valid_calls)==2
    # The selected bounds offsets survive every cdecl helper call up to the
    # numerical gate. Verify the actual remaining index instructions and that
    # no intervening instruction rewrites the two callee-saved offsets.
    assert next(r for r in spheres if r['address']==0x60)['data'].hex()=='8d34b6'
    assert next(r for r in spheres if r['address']==0x67)['data'].hex()=='c1e602'
    for row in spheres:
        if 0x6a<=row['address']<0x15d:
            first=row['operands'].split(',')[0].strip().upper()
            assert not (first in ('ESI','EDI') and row['mnemonic'] in ('MOV','LEA','ADD','SUB','XOR','INC','DEC','SHL','SHR','POP'))
    local=[]
    for phase,gate,hit,child_present in ((0,0,0,False),(0,1,1,False),(0,1,0,False),(0,1,0,True),(1,1,0,False),(1,1,0,True),(2,1,0,False)):
        name=f'phase{phase}-gate{gate}-hit{hit}-child{int(child_present)}'
        s=State(name,0x10b);s.path=list(base)
        s.reg.update(ESP=B(SP),EBX=B(Q),ESI=B(STACK)+8*d,EBP=B(0))
        s.store(Q,G);s.store(G,A);s.store(A+16,NODES);s.store(Q+36,capacity)
        s.store(B(NODES)+56*n+48,front);s.store(B(NODES)+56*n+52,back)
        s.store(B(STACK)+8*d-8,n);s.store(B(STACK)+8*d-4,phase)
        s.store(SP+0x4c,d);s.store(SP+0x54,bn);s.store(SP+0x58,transform);s.store(SP+0x5c,world)
        child=front if phase==0 else back
        if phase<2 and not hit and gate:s.path.append(child>=0 if child_present else child==-1)
        if phase<2 and not hit and gate and child_present:
            # In an acyclic N-node graph the current path plus a valid child
            # contains d+1 distinct nodes. capacity=N+1 therefore gives this
            # stronger bound; it must be preserved by the induction step.
            s.path.append(z.UGE(capacity,d+2))
        def hook(st,callee):
            sp=st.reg['ESP'];args=[st.load(sp+4*i) for i in range(6)]
            if callee=='_step':ret=1;assert feasible(st.path)
            elif callee=='_spheres':
                prove(name+'.sphere-arguments',st.path,z.And(args[0]==Q,args[1]==n,args[2]==bn,args[3]==transform,args[4]==1));ret=gate
                st.events.append(('gate',n,bn))
            elif callee=='_destination_tree':
                prove(name+'.destination-arguments',st.path,z.And(args[0]==Q,args[1]==n,args[2]==bn,args[3]==transform,args[4]==world,args[5]==SP+16));ret=hit
                st.events.append(('destination',n,bn))
            elif callee=='_mark_faces':
                delta=1 if phase==0 else -1
                prove(name+'.mark-arguments',st.path,z.And(args[0]==Q,args[1]==n,args[2]==B(delta)));ret=1
                st.events.append(('mark',n,delta))
            else:raise AssertionError((name,callee))
            # Arbitrary caller-saved clobbers make missing reloads visible.
            st.reg.update(EAX=B(ret),ECX=z.BitVec(name+'.clobber.ecx.'+str(st.pc),32),EDX=z.BitVec(name+'.clobber.edx.'+str(st.pc),32))
        paths=walk(execute,ecalls,s,{0x10b,0x22e},hook)
        for st in paths:
            constraints=st.path
            if phase==0 and hit:
                assert st.pc==0x22e;prove(name+'.hit-result',constraints,st.reg['EBP']==1)
                expected=[('gate',n,bn),('destination',n,bn)]
            else:
                newd=d-1 if phase==2 or not gate else d+1 if child_present else d
                prove(name+'.depth-and-frame-pointer',constraints,z.And(st.load(SP+0x4c)==newd,st.reg['ESI']==B(STACK)+8*newd))
                prove(name+'.capacity-invariant',constraints,z.UGE(capacity,newd+1))
                prove(name+'.miss-status',constraints,st.reg['EBP']==0)
                if phase<2 and gate:
                    prove(name+'.resume-phase',constraints,st.load(B(STACK)+8*d-4)==phase+1)
                    if child_present:prove(name+'.child-frame',constraints,z.And(st.load(B(STACK)+8*d)==child,st.load(B(STACK)+8*d+4)==0))
                expected=[]
                if phase==0:expected.append(('gate',n,bn))
                if phase==0 and gate:expected.extend([('destination',n,bn),('mark',n,1)])
                if phase==2:expected.append(('mark',n,-1))
            assert len(st.events)==len(expected)
            for a,b in zip(st.events,expected):
                assert a[0]==b[0]
                for x,y in zip(a[1:],b[1:]):prove(name+'.event',constraints,(B(x) if isinstance(x,int) else x)==(B(y) if isinstance(y,int) else y))
            local.append(dict(name=name,stop=hex(st.pc),covered=sorted(st.covered),effects=[e[0] for e in st.events]))
    # The remaining local lemmas are deliberately based on original instructions,
    # never on the optional recursive candidate.
    original_control=[]
    for contact in (0,1):
        s=State('original-destination.'+str(contact),0xab);s.path=list(base)
        s.reg.update(ESP=B(SP),ESI=n,EBX=bn)
        def destination_cut(st,callee):
            assert callee==0x44c200
            prove('original-destination-arguments.'+str(contact),st.path,z.And(st.load(st.reg['ESP'])==n,st.load(st.reg['ESP']+4)==bn))
            st.reg.update(EAX=B(contact),ECX=z.BitVec('original.dest.ecx',32),EDX=z.BitVec('original.dest.edx',32))
        for st in walk(original,ocalls,s,{0xc5,0xbe},destination_cut):
            assert st.pc==(0xbe if contact else 0xc5)
            prove('original-destination-result.'+str(contact),st.path,st.reg['EAX']==contact)
    s=State('original-gate-reject',0x17e);s.reg['ESP']=B(SP)
    for st in walk(original,ocalls,s,set(),lambda *_:(_ for _ in ()).throw(AssertionError('Unexpected original reject call'))):
        prove('original-rejected-gate-returns-zero',st.path,st.reg['EAX']==0)
    for first_result in (0,1):
        for second_result in (0,1):
            name=f'original-children-{first_result}-{second_result}'
            s=State(name,0x10f);s.path=list(base);s.reg.update(ESP=B(SP),EDI=B(NODES)+56*n,EBX=bn)
            s.store(B(NODES)+56*n+48,front);s.store(B(NODES)+56*n+52,back)
            def recurse(st,callee):
                assert callee==0x44d2f0
                ordinal=len(st.events);wanted=front if ordinal==0 else back
                prove(name+'.child-call',st.path,z.And(st.load(st.reg['ESP'])==wanted,st.load(st.reg['ESP']+4)==bn))
                st.events.append(('child',wanted));st.reg.update(EAX=B(first_result if ordinal==0 else second_result),ECX=B(0xaabb),EDX=B(0xccdd))
            for st in walk(original,ocalls,s,{0x12d},recurse):
                prove(name+'.short-circuit-return',st.path,st.reg['EAX']==(1 if first_result else second_result))
                assert len(st.events)==(1 if first_result else 2)
                original_control.append(dict(name=name,covered=sorted(st.covered),calls=len(st.events)))
    # A negative original child is an immediate false return. The iterative
    # implementation's omitted negative-child call is therefore stuttering.
    neg_a,neg_b=z.BitVecs('negative.a negative.b',32)
    s=State('original-negative');s.reg['ESP']=B(SP);saved=dict(s.reg)
    s.store(SP+4,neg_a);s.store(SP+8,neg_b);s.path=[z.Or(neg_a<0,neg_b<0)]
    for st in walk(original,ocalls,s,{0x1e},lambda *_:(_ for _ in ()).throw(AssertionError('Unexpected original call'))):
        assert st.returned
        prove('negative-child-is-zero',st.path,z.And(st.reg['EAX']==0,st.reg['ESP']==SP,
              *[st.reg[r]==saved[r] for r in ('EBX','ESI','EDI')]))
    # Actual source leaf helper establishes the pointer supplied to mark_faces.
    index,face,next_index,count,leaf_count,face_count=z.BitVecs('leaf.index leaf.face leaf.next count leaf.count face.count',32)
    list_domain=[index>=0,z.ULT(index,leaf_count),z.ULE(leaf_count,65535),z.UGE(leaf_count,1),
                 face>=0,z.ULT(face,face_count),z.ULE(face_count,65535),z.UGE(face_count,1),
                 z.Or(next_index==-1,z.And(next_index>=0,z.ULT(next_index,leaf_count))),z.ULE(count,1048575)]
    s=State('leaf-helper');s.path=list(list_domain);s.reg['ESP']=B(SP)
    for at,value in ((SP+4,Q),(SP+8,A),(SP+12,index),(SP+16,SP+0x100),
                     (A+20,LEAVES),(A+12,FACES),(A+44,leaf_count),(A+36,face_count)):
        s.store(at,value)
    s.store(B(LEAVES)+8*index,face)
    def leaf_step(st,callee):
        assert callee=='_step'
        prove('leaf.step-context',st.path,st.load(st.reg['ESP'])==Q)
        st.reg.update(EAX=B(1),ECX=z.BitVec('leaf.clobber.ecx',32),EDX=z.BitVec('leaf.clobber.edx',32))
    for st in walk(leaf,lcalls,s,set(),leaf_step):
        prove('leaf.pointer-and-result',st.path,z.And(st.load(SP+0x100)==B(LEAVES)+8*index,st.reg['EAX']==1))
    # Arbitrary one-leaf updates, including next-pointer preservation, are
    # proved independently from both original increment/decrement machine code.
    list_steps=[]
    for delta,old_start,old_stop in ((1,0xda,0xff),(-1,0x142,0x167)):
        name='list-'+str(delta)
        source=State(name+'.source',0x35);source.path=list(list_domain)+[count>0 if delta<0 else z.BoolVal(True)]
        source.reg.update(ESP=B(SP),EBX=B(A),ESI=B(Q),EDI=B(0),EBP=B(0))
        source.store(A+44,leaf_count);source.store(Q+32,SUPP);source.store(SP+0x1c,delta);source.store(SP+0x18,B(NODES)+56*n)
        source.store(B(LEAVES)+8*index,face);source.store(B(LEAVES)+8*index+4,next_index)
        source.store(B(SUPP)+4*face,count);source.reg['EAX']=index
        before_source=source.mem
        def item_leaf(st,callee):
            assert callee=='_leaf';sp=st.reg['ESP']
            prove(name+'.leaf-arguments',st.path,z.And(st.load(sp)==Q,st.load(sp+4)==A,st.load(sp+8)==index,st.load(sp+12)==SP+0x14))
            st.store(st.load(sp+12),B(LEAVES)+8*index)
            st.reg.update(EAX=B(1),ECX=z.BitVec(name+'.ecx',32),EDX=z.BitVec(name+'.edx',32))
        sources=walk(mark,mcalls,source,{0x35,0x80},item_leaf)
        old=State(name+'.original',old_start);old.path=list(list_domain)
        old.store(0x4e0fcc,0x130000);old.store(0x130000+0x64,LEAVES);old.store(0x130000+0x7c,FACES)
        old.store(B(LEAVES)+8*index,face);old.store(B(LEAVES)+8*index+4,next_index);old.store(B(FACES)+12*face+8,count)
        release_result=z.BitVec(name+'.saved-child-result',32)
        old.reg.update(EAX=index if delta>0 else release_result,ECX=index,ESI=B(0),EBX=B(0));before_old=old.mem
        originals=walk(original,ocalls,old,{old_start,old_stop},lambda *_:(_ for _ in ()).throw(AssertionError('Unexpected original list call')))
        for st in sources:
            prove(name+'.production-counter-next',st.path,z.And(st.load(B(SUPP)+4*face)==count+delta,st.reg['EAX']==next_index))
            other=z.BitVec(name+'.other-face',32)
            prove(name+'.production-frame',st.path+[z.ULT(other,face_count),other!=face],
                  st.load(B(SUPP)+4*other)==z.Select(before_source,B(SUPP)+4*other))
        for st in originals:
            prove(name+'.original-counter-next',st.path,z.And(st.load(B(FACES)+12*face+8)==count+delta,
                  st.reg['EAX' if delta>0 else 'ECX']==next_index))
            if delta<0:prove(name+'.original-preserves-child-result',st.path,st.reg['EAX']==release_result)
            other=z.BitVec(name+'.old-other-face',32)
            prove(name+'.original-frame',st.path+[z.ULT(other,face_count),other!=face],
                  st.load(B(FACES)+12*other+8)==z.Select(before_old,B(FACES)+12*other+8))
        list_steps.append(dict(delta=delta,source_paths=len(sources),original_paths=len(originals),
                               meaning='Arbitrary leaf index/face/count/next; one update agrees and every other face is unchanged'))
    # List-head selection is part of the proof: front precedes back, including
    # empty lists. This also establishes the induction base for each node.
    fh,bh=z.BitVecs('front-leaf-head back-leaf-head',32)
    heads=[z.Or(fh==-1,z.And(fh>=0,z.ULE(fh,65534))),z.Or(bh==-1,z.And(bh>=0,z.ULE(bh,65534)))]
    for origin,rows,calls,start,stops in ((False,mark,mcalls,0,{0x35}),(True,original,ocalls,0xc5,{0xda,0x10f})):
        s=State('list-entry.'+str(origin),start);s.path=list(base)+heads;s.reg['ESP']=B(SP)
        s.store(B(NODES)+56*n+40,fh);s.store(B(NODES)+56*n+44,bh)
        if origin:s.reg['EDI']=B(NODES)+56*n
        else:
            s.store(SP+4,Q);s.store(SP+8,n);s.store(SP+12,1);s.store(Q,G);s.store(G,A);s.store(A+16,NODES)
        for st in walk(rows,calls,s,stops,lambda *_:(_ for _ in ()).throw(AssertionError('Unexpected list-entry call'))):
            if st.returned or st.pc==0x10f:
                prove('list-entry.empty.'+str(origin),st.path,z.And(fh==-1,bh==-1))
            else:
                prove('list-entry.front-before-back.'+str(origin),st.path,z.And(st.reg['EAX']==z.If(fh>=0,fh,bh),
                      st.reg['ESI' if origin else 'EBP']==z.If(fh>=0,B(0),B(1))))
    # Source side transition and return, independently checked against original
    # post-list selection instructions; arbitrary empty/nonempty back lists.
    for origin,rows,calls,start,stops in ((False,mark,mcalls,0x80,{0x35}),(True,original,ocalls,0xff,{0xda,0x10f})):
        for side in (0,1):
            s=State('list-side.'+str(origin)+'.'+str(side),start);s.path=list(base)+heads;s.reg['ESP']=B(SP)
            s.store(B(NODES)+56*n+44,bh)
            if origin:s.reg.update(EDI=B(NODES)+56*n,ESI=B(side),EAX=B(-1))
            else:s.reg.update(ECX=B(NODES)+56*n,EBP=B(side))
            for st in walk(rows,calls,s,stops,lambda *_:(_ for _ in ()).throw(AssertionError('Unexpected list-side call'))):
                if st.returned or st.pc==0x10f:
                    prove('list-side.finished.'+str(origin)+'.'+str(side),st.path,z.Or(side==1,bh==-1))
                else:
                    prove('list-side.back.'+str(origin)+'.'+str(side),st.path,z.And(z.BoolVal(side==0),bh>=0,st.reg['EAX']==bh,
                          st.reg['ESI' if origin else 'EBP']==1))
    # Original decrement-list selection uses different live registers and
    # preserves EAX, the child return value, through every release iteration.
    original_status=z.BitVec('original.child-result',32)
    for entry,side in ((True,0),(False,0),(False,1)):
        s=State('original-release-head.'+str(entry)+str(side),0x12d if entry else 0x167)
        s.path=list(base)+heads+[z.Or(original_status==0,original_status==1)]
        s.reg.update(ESP=B(SP),EDI=B(NODES)+56*n,EBX=B(side),ECX=B(-1),EAX=original_status)
        s.store(B(NODES)+56*n+40,fh);s.store(B(NODES)+56*n+44,bh)
        for st in walk(original,ocalls,s,{0x142},lambda *_:(_ for _ in ()).throw(AssertionError('Unexpected release-selector call'))):
            prove('original-release-preserves-result.'+str(entry)+str(side),st.path,st.reg['EAX']==original_status)
            if st.returned:
                finished=z.And(fh==-1,bh==-1) if entry else z.Or(z.BoolVal(side==1),bh==-1)
                prove('original-release-finished.'+str(entry)+str(side),st.path,finished)
            else:
                desired=z.If(fh>=0,fh,bh) if entry else bh
                prove('original-release-head.'+str(entry)+str(side),st.path,z.And(st.reg['ECX']==desired,
                      st.reg['EBX']==(z.If(fh>=0,B(0),B(1)) if entry else B(1))))
    # Root initialization (after the explicitly separate finite/owner guards).
    for negative in (False,True):
        s=State('root-entry.'+str(negative),0xaa);s.reg.update(ESP=B(SP),EBX=B(Q),EBP=B(0))
        root,root_b=z.BitVecs('root.a root.b',32)
        s.path=[z.Or(root<0,root_b<0)] if negative else [root>=0,root_b>=0]
        s.store(SP+0x50,root);s.store(SP+0x54,root_b);s.store(SP+0x64,1);s.store(Q+24,STACK)
        for st in walk(execute,ecalls,s,{0x10b,0x22e},lambda *_:(_ for _ in ()).throw(AssertionError('Unexpected root-entry call'))):
            prove('root-entry.busy-and-adapter-fields.'+str(negative),st.path,z.And(st.load(Q+52)==1,st.load(Q+44)==0,st.load(Q+48)==0))
            if negative:
                assert st.pc==0x22e;prove('root-negative-result',st.path,st.reg['EBP']==0)
            else:
                assert st.pc==0x10b
                prove('root-initial-continuation',st.path,z.And(st.load(STACK)==root,st.load(STACK+4)==0,
                      st.load(SP+0x4c)==1,st.reg['ESI']==STACK+8))
    work=z.BitVec('work.before-step',32)
    s=State('step-valid');s.path=[z.ULE(work,1048575)];s.reg['ESP']=B(SP);s.store(SP+4,Q);s.store(Q+44,work)
    for st in walk(step,scalls,s,set(),lambda *_:(_ for _ in ()).throw(AssertionError('Unexpected bounded-step failure'))):
        prove('actual-work-budget-step',st.path,z.And(st.load(Q+44)==work+1,st.reg['EAX']==1))
    s=State('public-wrapper');s.reg['ESP']=B(SP)
    public_args=[z.BitVec('public.arg.'+str(i),32) for i in range(6)]
    for i,value in enumerate(public_args):s.store(SP+4+4*i,value)
    public_result=z.BitVec('public.result',32)
    def forward(st,callee):
        assert callee=='_execute'
        prove('public-wrapper-enters-actual-iterative-owner',st.path,
              z.And(*[st.load(st.reg['ESP']+4*i)==v for i,v in enumerate(public_args+[B(1)])]))
        st.reg.update(EAX=public_result,ECX=B(0xddee),EDX=B(0xeeff))
    for st in walk(wrapper,wcalls,s,set(),forward):
        prove('public-wrapper-publishes-result',st.path,z.And(st.reg['EAX']==public_result,st.reg['ESP']==SP))
    # Root publication/bulk cleanup is a stuttering replacement for the
    # original's now-unobservable ancestor releases after the first hit.
    nf,f=z.BitVecs('cleanup.face-count cleanup.face',32)
    cleanup=[]
    for status in (0,1):
        s=State('cleanup.'+str(status),0x22e);s.path=[z.UGE(nf,1),z.ULE(nf,65535),z.ULT(f,nf)]
        s.reg.update(ESP=B(SP),EBX=B(Q),EBP=B(status));s.store(Q,G);s.store(G,A);s.store(A+36,nf);s.store(Q+32,SUPP);s.store(SP+0x60,out)
        words=[z.BitVec('contact.word.'+str(i),32) for i in range(14)]
        for i,w in enumerate(words):s.store(SP+16+4*i,w)
        before=s.mem
        for st in walk(execute,ecalls,s,set(),lambda *_:(_ for _ in ()).throw(AssertionError('Unexpected cleanup call'))):
            prove('cleanup.status-busy-suppression.'+str(status),st.path,z.And(st.reg['EAX']==status,st.load(Q+52)==0,st.load(B(SUPP)+4*f)==0))
            for i,w in enumerate(words):
                prove('cleanup.output.'+str(status)+'.'+str(i),st.path,st.load(out+4*i)==(w if status else z.Select(before,out+4*i)))
            cleanup.append(dict(status=status,covered=sorted(st.covered)))
    ancestor,multiplicity=z.BitVecs('ancestor-count node-multiplicity',32)
    prove('suppression.push-pop-inverse',[],(ancestor+multiplicity)-multiplicity==ancestor)
    prove('root-final-release-restores-zero',[ancestor==multiplicity],ancestor-multiplicity==0)
    result=dict(passed=True,static_only=True,z3_version=z.get_version_string(),object=str(obj),object_sha256=digest(obj.read_bytes()),
        original_sha256=digest(image),original_owner='0044d2f0',original_bytes=391,
        production_execute_bytes=elen,production_mark_bytes=mlen,source_transitions=local,original_continuations=original_control,
        obligations=PROOFS,guarded_store_forwarding_queries=FORWARDING_QUERIES,candidate_read=False,list_item_refinement=list_steps,cleanup=cleanup,gate_expression=gate,native_code_equivalence=native_equivalence,
        induction=dict(relation='Each production frame(node,phase0/1/2) represents an original activation before the gate or with its saved front/back return continuation. A top phase1/2 has just observed a child miss/negative child; paused ancestors are waiting for that child. Marked frames contribute front+back leaf multiplicities to suppression',
            entry='Actual public wrapper forwards six inputs and source_tree=1; valid root starts one phase0 frame; negative roots return0',
            step='Universal SMT phase lemmas preserve depth/frame-pointer/child/frame-phase and observable destination order; omitted negative calls return0 in original',
            lists='Universal one-leaf lemma and front/back selectors establish finite-list induction, including shared tails and repeated face identities; increment/decrement are inverse',
            completion='Miss pops release current marks; hit has no later destination calls. Finite original ancestor releases return the zero initial suppression array; production bulk clear is stuttering cleanup with identical observable contact publication',
            composition='SMT discharges the actual machine local transitions and counter identities. The finite-list and activation-stack induction over these lemmas is explicitly reviewed here, not misrepresented as a sampled whole-program run or an automated termination theorem.',
            termination='Finite acyclic N-node topology: active path has d distinct nodes, capacity=N+1. A valid child implies d+1<=N and capacity>=d+2; pushed state preserves capacity>=new_depth+1. Finite list chains and work<=1048576; no claim outside these contracts'),
        limits=['Destination traversal/contact and matrix transform remain separately declared dependency contracts; no whole narrow-phase proof',
                'Source query owner/finite/guard validation and work counters are explicit adapters outside original ABI',
                'Proof is production scheduling/suppression refinement at the stated valid domain, not raw bytes or arbitrary runtime FP/invalid input equivalence'])
    (outdir/'continuations.json').write_text(json.dumps(result,indent=2)+'\n')
    (outdir/'production-execute.asm.txt').write_text(elisting);(outdir/'production-mark.asm.txt').write_text(mlisting)
    print(f'{len(PROOFS)} production/original continuation obligations proved UNSAT')


if __name__=='__main__':main()
