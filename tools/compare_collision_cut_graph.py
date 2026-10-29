"""Read-only original x87 plane-cut interpolation DAGs; no native code execution."""
import json
import re
import struct
import argparse
from pathlib import Path
from compare_ai_damage_query import image_bytes,digest,parse_coff,owner_instructions
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'META/m4/playable/collision-narrowphase'


def f32(v):return v if v[0] in ('input32','f32') else ('f32',v)
def op(kind,a,b):
    if kind in ('mul','add') and repr(a)>repr(b):a,b=b,a
    return (kind+'53',a,b)


def trace(rows,first,cached=False):
    d=('retained','current-distance');pd=('input32','previous-distance')
    previous=[('input32','previous.'+str(i)) for i in range(3)]
    current=[('input32','current.'+str(i)) for i in range(3)]
    sp,out,prev,cur=0x1000,0x2000,0x3000,0x4000
    reg={'ESP':sp,'EDI':out+4,'ECX':prev,'ESI':prev if cached else cur,'EDX':cur,'EBP':out+4,'EAX':0}
    mem={sp+0x10:pd,sp+0x14:0,0x489ea8:('constant',1.0.hex())}
    for i in range(3):mem[sp+0x6c+4*i]=mem[prev+4*i]=previous[i];mem[sp+0x78+4*i]=mem[cur+4*i]=current[i]
    if cached:
        mem[sp+0x14]=pd;mem[sp+0x18]=0
        for i in range(3):mem[sp+0x64+4*i]=previous[i];mem[sp+0x58+4*i]=current[i]
    fp=[d];writes=[]
    def address(arg):
        text=re.search(r'\[([^]]+)\]',arg)[1].upper().replace(' ','').replace('-','+-');value=0
        for term in text.split('+'):
            if term:value+=reg[term] if term in reg else int(term,0)
        return value
    def read(arg):
        arg=arg.upper();m=re.fullmatch(r'ST\(?([0-7])?\)?',arg)
        if m:return fp[int(m[1] or 0)]
        if '[' in arg:return mem[address(arg)]
        return reg[arg] if arg in reg else int(arg,0)
    for row in rows:
        m=row['mnemonic'];arg=', '.join(row['operands']).upper();p=[a.strip() for a in arg.split(',')];raw=bytes.fromhex(row['bytes'])
        if m=='FLD':fp.insert(0,read(arg))
        elif m=='FXCH':i=raw[1]&7;fp[0],fp[i]=fp[i],fp[0]
        elif m in ('FADD','FMUL','FSUB','FSUBR','FDIVR'):
            index=0;left,right=fp[0],read(p[-1])
            if len(p)==2:index=int(re.fullmatch(r'ST\(?([0-7])?\)?',p[0])[1] or 0);left=fp[index]
            if m in ('FSUBR','FDIVR'):left,right=right,left
            fp[index]=op({'FADD':'add','FMUL':'mul','FSUB':'sub','FSUBR':'sub','FDIVR':'div'}[m],left,right)
        elif m in ('FSUBP','FSUBRP','FADDP'):
            index=raw[1]&7;left,right=fp[index],fp[0]
            if m=='FSUBRP':left,right=right,left
            fp[index]=op('add' if m=='FADDP' else 'sub',left,right);fp.pop(0)
        elif m in ('FST','FSTP'):
            st=re.fullmatch(r'ST\(?([0-7])?\)?',arg)
            if st:fp[int(st[1] or 0)]=fp[0]
            else:
                at=address(arg);value=fp[0] if 'DOUBLE' in arg else f32(fp[0]);mem[at]=value
                writes.append(dict(address=row['address'],destination=hex(at),width=8 if 'DOUBLE' in arg else 4,expression=value))
            if m=='FSTP':fp.pop(0)
        elif m=='MOV':
            if '[' in p[0]:mem[address(p[0])]=read(p[1])
            else:reg[p[0]]=read(p[1])
        elif m=='INC':reg[arg]+=1
        elif m=='ADD':reg[p[0]]+=read(p[1])
        else:raise AssertionError((row['address'],m,arg))
    inverse=op('div',('constant',1.0.hex()),op('sub',d,pd))
    expected=[];old=[]
    for i in range(3):
        left=op('mul',d,previous[i]);right=op('mul',pd,current[i])
        old.append(f32(op('mul',op('sub',left,right),inverse)))
        if not first:left=f32(left);right=f32(right) if (i==2 if cached else i>0) else right
        expected.append(f32(op('mul',op('sub',left,right),inverse)))
    actual=[mem[out+4*i] for i in range(3)]
    assert actual==expected,(first,actual,expected)
    return dict(owner='0044cc20' if cached else '0044c200',first=first,actual=actual,previous_implementation=old,previous_graph_matches=(actual==old),writes=writes)


def source_trace(rows,constant_calls,first,cached,start,stop):
    sp,frame,out=0x10000,0x10200,0x20000
    reg=dict(ESP=sp,EBP=frame,EDI=0,ESI=0,EAX=0,ECX=0,EDX=0)
    d=('retained','current-distance');pd=('input32','previous-distance')
    mem={sp+0x10:pd,sp+0x20:d,sp+0x24:('high64',d),frame+0x1c:int(first),frame+0x20:out,frame+0x24:int(cached)}
    for i in range(3):mem[sp+0x48+4*i]=('input32','previous.'+str(i));mem[sp+0x3c+4*i]=('input32','current.'+str(i))
    lookup={r['address']:r for r in rows};pc=start;fp=[];zero=carry=False;covered=[]
    def address(arg):
        value=0
        for term in re.search(r'\[([^]]+)\]',arg)[1].upper().replace(' ','').replace('-','+-').split('+'):
            if not term:continue
            product=1
            for f in term.split('*'):product*=reg[f] if f in reg else int(f,0)
            value+=product
        return value
    def read(arg):
        arg=arg.upper();st=re.fullmatch(r'ST\(?([0-7])?\)?',arg)
        if st:return fp[int(st[1] or 0)]
        if '[' in arg:return mem[address(arg)]
        return reg[arg] if arg in reg else int(arg,0)
    def write(arg,value,width=4):
        if arg in reg:reg[arg]=value
        else:
            at=address(arg);mem[at]=value
            if width==8:mem[at+4]=('high64',value)
    for _ in range(300):
        if pc==stop:
            assert not fp
            return dict(actual=[mem[out+4*i] for i in range(3)],covered=covered)
        row=lookup[pc];m=row['mnemonic'];arg=row['operands'].upper();p=[v.strip() for v in arg.split(',')];following=pc+len(row['data']);covered.append(pc)
        if m=='MOV':write(p[0],read(p[1]))
        elif m=='LEA':write(p[0],address(p[1]))
        elif m=='PUSH':value=read(arg);reg['ESP']-=4;mem[reg['ESP']]=value
        elif m in ('ADD','SUB'):reg[p[0]]+=read(p[1])*(1 if m=='ADD' else -1)
        elif m=='XOR':assert p[0]==p[1];reg[p[0]]=0
        elif m=='INC':reg[arg]+=1
        elif m in ('TEST','CMP'):
            a,b=read(p[0]),read(p[1]);zero=(a&b if m=='TEST' else a-b)==0;carry=a<b
        elif m in ('JNE','JE','JB','JMP'):
            if m=='JMP' or (not zero if m=='JNE' else zero if m=='JE' else carry):following=int(arg,0)
        elif m=='CALL':
            assert constant_calls[pc]=='round'
            dest=mem[reg['ESP']];value=mem[reg['ESP']+4]
            assert mem[reg['ESP']+8]==('high64',value)
            # Exact actual helper11B is independently verified below.
            mem[dest]=f32(value);reg['EAX']=dest
        elif m=='FLD':fp.insert(0,read(arg))
        elif m=='FXCH':i=row['data'][1]&7;fp[0],fp[i]=fp[i],fp[0]
        elif m in ('FMUL','FSUB','FSUBR','FDIVR'):
            index=0;left=fp[0];right=constant_calls[pc] if pc in constant_calls else read(p[-1])
            if len(p)==2:index=int(re.fullmatch(r'ST\(?([0-7])?\)?',p[0])[1] or 0);left=fp[index]
            if m in ('FSUBR','FDIVR'):left,right=right,left
            fp[index]=op({'FMUL':'mul','FSUB':'sub','FSUBR':'sub','FDIVR':'div'}[m],left,right)
        elif m in ('FSUBP','FSUBRP'):
            i=row['data'][1]&7;left,right=fp[i],fp[0]
            if m=='FSUBRP':left,right=right,left
            fp[i]=op('sub',left,right);fp.pop(0)
        elif m in ('FST','FSTP'):
            width=8 if 'QWORD' in arg else 4;write(arg,fp[0] if width==8 else f32(fp[0]),width)
            if m=='FSTP':fp.pop(0)
        else:raise AssertionError((hex(pc),m,arg))
        pc=following
    raise AssertionError('Unbounded source slice')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--object',type=Path,default=ROOT/'build/collision-full-original/bh_collision_model.obj')
    parser.add_argument('--output',type=Path,default=OUT/'original-cut-graphs.json')
    args=parser.parse_args()
    image=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert digest(image)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    graphs=[]
    for cached,first,start,stop in ((False,True,0x44c4a4,0x44c524),(False,False,0x44c7ae,0x44c86d),
                                   (True,True,0x44cd4b,0x44cdc8),(True,False,0x44cfcc,0x44d083)):
        path='META/m4/face-contact/original/disassembly.json' if cached else 'META/m4/collision-traversal/original/0044c200.disassembly.json'
        rows=json.loads((ROOT/path).read_text())
        selected=[r for r in rows if start<=int(r['address'],16)<stop]
        assert b''.join(bytes.fromhex(r['bytes']) for r in selected)==image_bytes(image,start,stop-start)
        graphs.append(trace(selected,first,cached))
    symbols,sections=parse_coff(args.object.read_bytes());symbol=next(s for s in symbols.values() if s['name']=='_face_cut')
    source,listing,end,_=owner_instructions(args.object,symbols,sections,'_face_cut');section=sections[symbol['section']]
    div_index=next(i for i,r in enumerate(source) if r['mnemonic']=='FDIVR')
    start=source[div_index-3]['address']
    stop=next(r['address'] for r in source if r['mnemonic']=='ADD' and r['operands']=='edi, 0x3')
    assert [r['mnemonic'] for r in source[div_index-3:div_index]]==['FLD','FSUBR','XOR']
    calls={}
    for rel in section['relocations']:
        row=next(r for r in source if r['address']<=rel['offset']<r['address']+len(r['data']))
        if not start<=row['address']<stop:continue
        target=rel['symbol']
        if rel['kind']==20:assert target['name']=='_bh_round_float';calls[row['address']]='round'
        else:
            assert rel['kind']==6
            at=target['value']+struct.unpack_from('<I',section['data'],rel['offset'])[0]
            assert sections[target['section']]['data'][at:at+8]==struct.pack('<d',1.)
            calls[row['address']]=('constant',1.0.hex())
    hp=ROOT/'build/collision-full-original/bh_round.obj';hs,hsections=parse_coff(hp.read_bytes())
    helper,_,_,_=owner_instructions(hp,hs,hsections,'_bh_round_float')
    assert b''.join(r['data'] for r in helper).hex()=='dd4424088b442404d918c3'
    for original in graphs:
        proof=source_trace(source,calls,original['first'],original['owner']=='0044cc20',start,stop)
        assert original['actual']==proof['actual'],('Actual compiled source differs',original['owner'],original['first'],original['actual'],proof['actual'])
        original['compiled_source']=proof
    OUT.mkdir(parents=True,exist_ok=True)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(dict(passed=True,static_only=True,graphs=graphs,source_object=str(args.object),source_object_sha256=digest(args.object.read_bytes()),source_helper_bytes=end,
        scope='Actual original retained PC53 cut expressions, with explicit binary32 stores; no reassociation or native fixture-derived expectations'),indent=2)+'\n')
    print('All four actual original/source interpolation graphs agree; cached and inline second-cut variants remain separate')


if __name__=='__main__':main()
