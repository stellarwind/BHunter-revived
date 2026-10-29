"""Static promotion x87/branch/store/call graphs. Never executes game or oracle code."""
import argparse
import csv
import itertools
import json
import re
import struct
import sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from compare_ai_pose import instructions
from compare_ai_guided import digest
from compare_ai_center import image_bytes
from compare_ai_promotion import prefix_trace
sys.path.insert(0,str(ROOT/'analysis/astra-review'))
from targeted_probes import parse_coff


def trace(rows,constants,calls,source,image,out_direction,in_direction,same_port,flags,bits,fail_conversion=0):
    def ptr(name,offset=0):return ('ptr',name,offset)
    def c(v):return ('constant',float(v).hex())
    def f32(v):
        if isinstance(v,tuple):
            if v[0]=='f32' or v[0]=='input-float':return v
            if v[0]=='constant':return c(struct.unpack('<f',struct.pack('<f',float.fromhex(v[1])))[0])
        return ('f32',v)
    def op(name,a,b):
        if name in ('add','mul') and repr(a)>repr(b):a,b=b,a
        return (name+'53',a,b)
    def add(v,n):return ptr(v[1],v[2]+n) if isinstance(v,tuple) and v[0]=='ptr' else v+n
    def low16(v):return v&65535 if isinstance(v,int) else v if v==('clock-word',) or v[0]=='low16' else ('low16',v)
    outgoing=0 if same_port else 1
    memory={4:ptr('actor'),8:ptr('binding') if source else 0,12:0 if source else outgoing,16:outgoing,
            ('actor',0xf8):0,('actor',0xf4):0,('actor',0x1a4):ptr('old'),('actor',0x1a8):ptr('world'),
            ('old',0x128):ptr('actor'),('world',0x128):0,
            ('binding',0):ptr('context'),('binding',4):ptr('old-record'),('binding',8):ptr('new-record'),
            ('binding',12):ptr('world'),('binding',16):ptr('portal',0),('binding',20):ptr('portal',60*outgoing),
            ('world',0x114):flags,('clock',0):('clock-word',)}
    for offset in list(range(0x120,0x15c,4))+[0x180,0x184,0x188,0x18c,0x190,0x198]:memory[('actor',offset)]=('input-float','actor.'+hex(offset))
    for i in range(6):memory[('world',0x1b0+4*i)]=('input-float','bound.'+str(i))
    for index in range(2):
        memory[('portal',60*index)]=out_direction if index==outgoing else in_direction
        memory[('portal',60*index+4)]=('input-integer','plane.'+str(index))
        memory[('portal',60*index+56)]=ptr('neighbor.'+str(index))
        for j in range(4):memory[('portal',60*index+40+4*j)]=('input-float','aperture.'+str(index)+'.'+str(j))
    regs=dict(ESP=0,ESI='saved.esi',EDI='saved.edi',EBX='saved.ebx',EBP='saved.ebp')
    stack=[];events=[];comparisons=[];covered=set();cursor=0;flags_equal=None;comparison=None;carry=False
    lookup={r['address']:i for i,r in enumerate(rows)};axis_count=0;conversion_count=0
    def canonical(v):
        if isinstance(v,int):return v
        if v[0] in ('clock','absolute'):return v
        assert v[0]=='ptr',v
        name,offset=v[1:]
        if name=='stack':return offset
        if name in ('old-record','new-record') and offset==4:return ('old' if name=='old-record' else 'world',0x128)
        if name=='world':
            if source:
                if 0x1e8<=offset<0x200:return (name,offset-0x1e8+0x1b0)
                if offset==0x204:return (name,0x114)
            elif 0x1cc<=offset<0x1cc+120:return ('portal',offset-0x1cc)
        return (name,offset)
    def address(operand):
        inner=re.search(r'\[(.*?)\]',operand)[1].strip()
        if inner.startswith('0X') and '+' not in inner:
            address_=int(inner,16)
            if address_==0x4dfac4:return ('clock',0)
            return ('absolute',address_)
        value_=0
        for term in inner.split('+'):
            term=term.strip()
            if '*' in term:
                a,b=[v.strip() for v in term.split('*')];item=(regs[a] if a in regs else int(a,16))*(regs[b] if b in regs else int(b,16))
            else:item=regs[term] if term in regs else int(term,16)
            if isinstance(item,tuple):
                assert not isinstance(value_,tuple),('Two pointer terms',inner,regs)
                value_=add(item,value_)
            else:value_=add(value_,item)
        return value_
    def reg_value(name):
        aliases={'AX':'EAX','CX':'ECX','DX':'EDX'}
        return low16(regs[aliases[name]]) if name in aliases else regs[name]
    def st(operand):
        m=re.fullmatch(r'ST\(?([0-7])?\)?',operand);return int(m[1] or '0') if m else None
    def read(operand,row,floating=False):
        index=st(operand)
        if index is not None:return stack[index]
        if '[' in operand:
            if row['address'] in constants:return constants[row['address']]
            at=canonical(address(operand));v=memory[at]
            if floating and ('QWORD' in operand or 'DOUBLE' in operand) and isinstance(v,tuple) and v[0]=='low64':
                assert memory[at+4]==('high64',v[1]);v=v[1]
        elif operand in regs or operand in ('AX','CX','DX'):v=reg_value(operand)
        else:v=int(operand,16)&0xffffffff
        if floating and isinstance(v,int):return c(struct.unpack('<f',struct.pack('<I',v))[0])
        return v
    def write(operand,v,row,floating=False):
        index=st(operand)
        if index is not None:stack[index]=v;return
        if '[' not in operand:
            aliases={'AX':'EAX','CX':'ECX','DX':'EDX'};regs[aliases.get(operand,operand)]=low16(v) if operand in aliases else v;return
        at=canonical(address(operand));wide=floating and ('QWORD' in operand or 'DOUBLE' in operand)
        if floating and not wide:v=f32(v)
        if 'WORD PTR' in operand and 'DWORD' not in operand and 'QWORD' not in operand:v=low16(v)
        if wide:
            assert isinstance(at,int),('Unexpected non-stack double store',at)
            memory[at]=('low64',v);memory[at+4]=('high64',v)
        else:memory[at]=v
        if isinstance(at,tuple) and at[0] in ('actor','old','world'):
            if at[0]=='actor' and 0x120<=at[1]<=0x198 and isinstance(v,int):v=c(struct.unpack('<f',struct.pack('<I',v))[0]);memory[at]=v
            events.append(('store',at,v))
    def vector(p):return tuple(memory[canonical(add(p,4*i))] for i in range(3))
    while cursor<len(rows):
        row=rows[cursor];cursor+=1;covered.add(row['address']);m,arg=row['mnemonic'],row['operands'].upper();parts=[p.strip() for p in arg.split(',')]
        if m=='MOV':write(parts[0],read(parts[1],row),row)
        elif m=='LEA':
            v=address(parts[1]);write(parts[0],ptr('stack',v) if isinstance(v,int) and 'ESP' in parts[1] else v,row)
        elif m=='PUSH':v=read(arg,row);regs['ESP']-=4;memory[regs['ESP']]=v
        elif m=='POP':regs[arg]=memory[regs['ESP']];regs['ESP']+=4
        elif m in ('ADD','SUB'):
            v=add(read(parts[0],row),int(parts[1],16)*(1 if m=='ADD' else -1));write(parts[0],v,row);flags_equal=v==0
        elif m=='DEC':v=read(arg,row)-1;write(arg,v,row);flags_equal=v==0
        elif m in ('AND','XOR'):
            a,b=read(parts[0],row),read(parts[1],row);v=a&b if m=='AND' else a^b;write(parts[0],v,row);flags_equal=v==0
        elif m=='CMP':a,b=read(parts[0],row),read(parts[1],row);flags_equal=a==b;carry=a<b
        elif m=='TEST':
            if parts[0]=='AH':
                assert comparison is not None
                mask=int(parts[1],16);assert mask in (1,0x41)
                comparisons.append((mask,*comparison));flags_equal=not bool(bits&(1<<(len(comparisons)-1)))
            else:flags_equal=(read(parts[0],row)&read(parts[1],row))==0
        elif m in ('JZ','JE','JNZ','JNE','JA','JMP'):
            take=m=='JMP' or (flags_equal if m in ('JZ','JE') else not carry and not flags_equal if m=='JA' else not flags_equal)
            if take:
                if '[' in arg:
                    at=canonical(address(arg));table=at if isinstance(at,int) else at[1];assert table>=0x400000
                    target=struct.unpack('<I',image_bytes(image,table,4))[0]-0x434f40
                else:target=int(arg,16)
                cursor=lookup[target]
        elif m in ('FLD','FILD'):v=read(arg,row,m=='FLD');stack.insert(0,('i2f',v) if m=='FILD' else v)
        elif m=='FXCH':index=st(arg) if arg else 1;stack[0],stack[index]=stack[index],stack[0]
        elif m=='FCHS':stack[0]=('negate',stack[0])
        elif m in ('FST','FSTP'):
            write(arg,stack[0],row,True)
            if m=='FSTP':stack.pop(0)
        elif m in ('FADD','FSUB','FSUBR','FMUL','FDIV','FDIVR'):
            index=st(parts[0]) if len(parts)==2 else 0;left,right=stack[index],read(parts[-1],row,True)
            if m in ('FSUBR','FDIVR'):left,right=right,left
            stack[index]=op({'FADD':'add','FSUB':'sub','FSUBR':'sub','FMUL':'mul','FDIV':'div','FDIVR':'div'}[m],left,right)
        elif m in ('FADDP','FSUBP','FSUBRP','FMULP','FDIVP','FDIVRP'):
            index=st(parts[0]) if arg else 1;left,right=stack[index],stack[0]
            if m in ('FSUBRP','FDIVRP'):left,right=right,left
            stack[index]=op({'FADDP':'add','FSUBP':'sub','FSUBRP':'sub','FMULP':'mul','FDIVP':'div','FDIVRP':'div'}[m],left,right);stack.pop(0)
        elif m in ('FCOM','FCOMP','FCOMPP'):
            comparison=(stack[0],stack[1] if m=='FCOMPP' else read(arg,row,True))
            if m!='FCOM':stack.pop(0)
            if m=='FCOMPP':stack.pop(0)
        elif m=='FNSTSW':assert arg=='AX';regs['EAX']=('statusword',comparison)
        elif m=='NEG':v=read(arg,row);assert isinstance(v,int);carry=v!=0;write(arg,-v,row)
        elif m=='SBB':assert parts[0]==parts[1];write(parts[0],-int(carry),row)
        elif m=='CALL':
            role=calls[row['address']];result=('clobbered',)
            if role=='axis':
                assert not stack;point=memory[regs['ESP']];events.append(('axis',vector(point)))
                for i in range(3):memory[canonical(add(point,4*i))]=('input-float','axis.'+str(axis_count)+'.'+str(i))
                stack.append(('axis-return',axis_count));axis_count+=1
            elif role=='ftol':
                v=stack.pop(0);events.append(('ftol',v));result=('ftol-low32',v)
            elif role=='ticks':
                assert not stack
                lo,hi,dest=[memory[regs['ESP']+4*i] for i in range(3)];assert lo[0]=='low64' and hi==('high64',lo[1])
                conversion_count+=1
                if fail_conversion==conversion_count:result=0
                else:
                    v=op('mul',lo[1],c(30));events.append(('ftol',v));memory[canonical(dest)]=low16(('ftol-low32',v));result=1
            elif role=='clock':assert not stack;result=('clock-word',)
            else:assert role=='finite' and not stack;result=1
            regs.update(EAX=result,ECX=('clobbered',),EDX=('clobbered',))
        elif m=='RET':
            assert regs['ESP']==0 and not stack
            assert all(regs[r]=='saved.'+r.lower() for r in ('ESI','EDI','EBX','EBP'))
            if source:assert regs['EAX']==(0 if fail_conversion else 1)
            return events,comparisons,covered
        else:raise AssertionError(('Unreviewed instruction',row))
    raise AssertionError('No complete return')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--object',type=Path,default=ROOT/'build/game/bh_actor_route.obj')
    parser.add_argument('--output-directory',type=Path,default=ROOT/'META/m4/playable/ai-byte-match/promotion-math')
    args=parser.parse_args();out=args.output_directory;out.mkdir(parents=True,exist_ok=True)
    image=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert digest(image)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    original=[]
    for r in csv.DictReader((ROOT/'META/m4/target-steering/original/00434f40.tsv').open(),delimiter='\t'):
        va=int(r['address'],16);data=bytes.fromhex(r['bytes']);assert data==image_bytes(image,va,len(data));m,_,arg=r['instruction'].partition(' ')
        if m.startswith('J') and '[' not in arg:arg=hex(int(arg,16)-0x434f40)
        original.append(dict(address=va-0x434f40,data=data,mnemonic=m,operands=arg))
    assert b''.join(r['data'] for r in original)==image_bytes(image,0x434f40,1600)
    path=args.object;obj=path.read_bytes();symbols,sections=parse_coff(obj)
    owner=next(s for s in symbols.values() if s['name']=='_bh_actor_route_promote_core');section=sections[owner['section']]
    # Reviewed checked adapter, independent of the caller graph shortcut:
    # scale by30, reject nonfinite/out-of-signed64 range, invoke the real ftol,
    # store onlyAX, return1. All alternate branches return0 before that store.
    helper=next(s for s in symbols.values() if s['name']=='_duration_ticks');h=sections[helper['section']]
    assert h['data'].hex()=='dd442404dc0d00000000dd5c24048b4424088b4c24045051e80000000083c40885c07438dd442404dc1d00000000dfe0f6c4017527dd442404dc1d00000000dfe0f6c4017416dd442404e8000000008b54240c668902b801000000c333c0c390'
    assert [(r['offset'],r['kind']) for r in h['relocations']]==[(6,6),(25,20),(42,6),(59,6),(75,20)]
    assert [r['symbol']['name'] for r in h['relocations'] if r['kind']==20]==['__finite','__ftol']
    helper_constants=[]
    for r in h['relocations']:
        if r['kind']==6:
            s=r['symbol'];at=s['value']+struct.unpack_from('<I',h['data'],r['offset'])[0]
            helper_constants.append(struct.unpack('<d',sections[s['section']]['data'][at:at+8])[0])
    assert helper_constants==[30.0,-9223372036854775808.0,9223372036854775808.0]
    source,listing=instructions(path,owner['name']);end=max(r['address']+len(r['data']) for r in source if r['mnemonic']=='RET');source=[r for r in source if r['address']<end]
    assert b''.join(r['data'] for r in source)==section['data'][:end]
    def const(data):return ('constant',struct.unpack('<d' if len(data)==8 else '<f',data)[0].hex())
    old_constants={};old_calls={};new_constants={};new_calls={}
    for r in original:
        if r['mnemonic']=='CALL':old_calls[r['address']]={0x4343c0:'axis',0x47de20:'ftol'}[0x434f40+r['address']+5+struct.unpack_from('<i',r['data'],1)[0]]
        elif r['mnemonic'].startswith('F'):
            m=re.search(r'\[0x([0-9a-f]+)\]',r['operands'])
            if m:old_constants[r['address']]=const(image_bytes(image,int(m[1],16),8 if 'double' in r['operands'] else 4))
    for relocation in section['relocations']:
        offset,s=relocation['offset'],relocation['symbol'];row=next(r for r in source if r['address']<=offset<r['address']+len(r['data']))
        if relocation['kind']==20:new_calls[row['address']]={'_bh_vec3_snap_axis':'axis','_duration_ticks':'ticks','_bh_actor_route_now':'clock','__finite':'finite'}[s['name']]
        else:
            assert relocation['kind']==6 and row['mnemonic'].startswith('F')
            width=8 if 'QWORD' in row['operands'].upper() else 4;at=s['value']+struct.unpack_from('<I',section['data'],offset)[0]
            new_constants[row['address']]=const(sections[s['section']]['data'][at:at+width])
    proofs=[];old_covered=set();new_covered=set()
    directions=(0,1,2,4,5,6)
    routes=[(o,i,False,f) for o,i,f in itertools.product(directions,directions,(0,8))]+[(o,o,True,f) for o,f in itertools.product(directions,(0,8))]
    for route in routes:
        for bits in range(128):
            old,oc,ov=trace(original,old_constants,old_calls,False,image,*route,bits)
            new,nc,nv=trace(source,new_constants,new_calls,True,image,*route,bits)
            if old!=new or oc!=nc:
                (out/'difference.json').write_text(json.dumps(dict(route=route,width_comparison_bits=bits,original=old,source=new,original_comparisons=oc,source_comparisons=nc),indent=2)+'\n')
                raise AssertionError('Promotion graph mismatch; see promotion-math/difference.json')
            old_covered|=ov;new_covered|=nv
            proofs.append(dict(route=route,width_bits=bits,graph_sha256=digest(json.dumps((old,oc),sort_keys=True).encode())))
    # List shapes factor independently from the arithmetic: no call occurs
    # before these identical, ordered list/ownership/start-image stores.
    prefix_cases=0
    for same in (False,True):
        for previous,next_ in itertools.product((False,True),repeat=2):
            for head in ((False,) if same else (False,True)):
                old,oc=prefix_trace(original,False,previous,next_,head,same)
                new,nc=prefix_trace(source,True,previous,next_,head,same)
                assert old==new;old_covered|=oc;new_covered|=nc;prefix_cases+=1
    failures=[]
    for which in (1,2):
        events,comparisons,covered=trace(source,new_constants,new_calls,True,image,0,1,False,0,127,which)
        new_covered|=covered
        assert sum(e[0]=='ftol' for e in events)==which-1
        failures.append(dict(failed_conversion=which,returns_zero_with_balanced_stack=True,graph_sha256=digest(json.dumps((events,comparisons)).encode())))
    original_uncovered=[hex(r['address']) for r in original if r['address'] not in old_covered]
    source_uncovered=[hex(r['address']) for r in source if r['address'] not in new_covered]
    assert original_uncovered==['0x20a','0x20c'] # invalid portal direction discards two live x87 temporaries
    assert not source_uncovered
    result=dict(original_va='00434f40',original_body_bytes=1600,source_body_bytes=end,byte_match=False,
                complete_valid_path_graphs_equal=True,path_cases=len(proofs),object_sha256=digest(obj),checked_duration_adapter_audited=True,
                prefix_cases=prefix_cases,source_guard_failure_traces=failures,all_source_instructions_covered=True,
                original_uncovered=original_uncovered,original_exclusion='Only4 bytes discarding two x87 values on invalid portal directions, rejected by checked source portal_valid before mutation.',
                source_uncovered=source_uncovered,proofs=proofs,
                limits=['PC53 nearest/masked; sum/product commutation and identity binary64 spills only.',
                        'Real axis contract is compared by all input/output bindings, not its internal implementation.',
                        'Source duration guard instruction body, constants, finite/ftol dependencies and both rejection returns are audited separately.',
                        'Source final finite guard and invalid-direction rejection have no original valid-path counterpart.'])
    (out/'graphs.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='proofs'}))


if __name__=='__main__':main()
