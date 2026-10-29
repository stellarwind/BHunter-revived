"""Read-only PC53 compiled-instruction algebra for collision-cache successful paths.

No native code execution, integration runner, or source-derived numerical oracle.
The original EXE and the current real COFF TU are independently decoded.
"""
import json
import re
import struct
from pathlib import Path
from compare_ai_damage_query import owner_instructions, parse_coff, image_bytes, digest

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'META/m4/playable/collision-close/cache'


def constant(value):
    return ('constant', float(value).hex())


def f32(value):
    if value[0] in ('input32', 'f32'):
        return value
    return ('f32', value)


def operation(name, a, b):
    # Only commutation of a single finite rounded operation; never reassociate.
    if name in ('add', 'mul') and repr(a) > repr(b):
        a, b = b, a
    return (name + '53', a, b)


def trace(rows, constants, calls, helpers, source, faces, nodes, lazy=False, explicit=False):
    model, bsp, face_data, node_data, allocator = 0x100000, 0x110000, 0x300000, 0x200000, 0x120000
    arrays = [0x400000 + 0x10000 * i for i in range(5)]
    reg = dict(ESP=0, EBP=0x777777, EBX=0x555555, ESI=0x666666, EDI=0x888888,
               EAX=0xaaaaaa, ECX=0xbbbbbb, EDX=0xcccccc)
    initial = reg.copy()
    mem = {0: -1, 4: model, 8: bsp, 12: allocator if explicit else 0}
    alloc_events, visited, writes, loops = [], set(), [], []
    if source:
        mem.update({model: bsp, model+28: arrays[1] if lazy else 0,
                    bsp+12: face_data, bsp+16: node_data, bsp+36: faces, bsp+40: nodes})
        for base in (allocator, bsp+68):
            mem.update({base: 0x900000, base+4: 0x900004, base+8: 0x900008})
    else:
        mem.update({model+0x6c: arrays[1] if lazy else 0, model+0x50: faces,
                    model+0x54: face_data, model+0x58: nodes, model+0x5c: node_data})
    for i in range(faces):
        for j in range(3):
            mem[face_data+12*i+4*j] = ('face-word', i, j)
    for i in range(nodes):
        for j in range(6):
            mem[node_data+56*i+4*j] = ('input32', i, j)
    fp, frames = [], []
    zero = carry = negative = False
    scope, pc = 'owner', 0
    scopes = {'owner': (rows, constants, calls), **helpers}

    def address(text):
        inside = re.search(r'\[([^]]+)\]', text)[1].upper().replace(' ', '')
        inside = inside.replace('-', '+-')
        total = 0
        for term in inside.split('+'):
            if not term:
                continue
            if '*' in term:
                a, b = term.split('*')
                total += (reg[a] if a in reg else int(a, 0)) * (reg[b] if b in reg else int(b, 0))
            else:
                total += reg[term] if term in reg else int(term, 0)
        return total

    def read(text, floating=False, width=4):
        text = text.strip().upper()
        m = re.fullmatch(r'ST\(?([0-7])?\)?', text)
        if m:
            return fp[int(m[1] or 0)]
        if text in reg:
            return reg[text]
        if '[' in text:
            addr = address(text)
            value = mem[addr]
            if floating and width == 8 and addr not in absolute:
                assert mem[addr+4] == ('high64', value), (scope, hex(pc), addr)
            return value
        return int(text, 0)

    def write(text, value, width=4):
        text = text.strip().upper()
        if text in reg:
            reg[text] = value
        else:
            assert '[' in text, (scope,hex(pc),text,reg)
            addr = address(text)
            mem[addr] = value
            if width == 8:
                mem[addr+4] = ('high64', value)
            if addr >= 0x100000:
                writes.append((addr, value))

    absolute = set()
    for scope_constants in [constants] + [h[1] for h in helpers.values()]:
        for addr, value in scope_constants.items():
            mem[addr] = value
            absolute.add(addr)
    for budget in range(100000):
        current, _, current_calls = scopes[scope]
        lookup = {row['address']: i for i, row in enumerate(current)}
        row = current[lookup[pc]]
        m, arg = row['mnemonic'], row['operands'].upper()
        parts = [v.strip() for v in arg.split(',')]
        visited.add((scope, pc))
        following = pc + len(row['data'])
        if m == 'PUSH':
            value = read(arg);reg['ESP'] -= 4;mem[reg['ESP']] = value
        elif m == 'POP':
            write(arg, mem[reg['ESP']]);reg['ESP'] += 4
        elif m == 'MOV':
            write(parts[0], read(parts[1]))
        elif m == 'LEA':
            write(parts[0], address(parts[1]))
        elif m in ('ADD', 'SUB', 'AND', 'SHL'):
            a, b = read(parts[0]), read(parts[1])
            value = a+b if m=='ADD' else a-b if m=='SUB' else a&b if m=='AND' else a<<b
            write(parts[0], value);zero = value==0;negative = value<0
        elif m == 'XOR':
            assert parts[0] == parts[1];write(parts[0], 0);zero = True
        elif m in ('INC', 'DEC'):
            value = read(arg)+(1 if m=='INC' else -1);write(arg,value);zero=value==0;negative=value<0
        elif m in ('CMP', 'TEST'):
            a, b = read(parts[0]), read(parts[1])
            value = a-b if m=='CMP' else a&b
            zero, carry, negative = value==0, a<b, value<0
        elif m in ('JE','JZ','JNE','JNZ','JA','JAE','JB','JBE','JL','JLE','JGE','JG','JMP'):
            take = {'JE':zero,'JZ':zero,'JNE':not zero,'JNZ':not zero,'JA':not carry and not zero,
                    'JAE':not carry,'JB':carry,'JBE':carry or zero,'JL':negative,'JLE':negative or zero,
                    'JGE':not negative,'JG':not negative and not zero,'JMP':True}[m]
            if take:
                following = int(arg, 0)
                if following < pc:
                    loops.append((scope, pc, following))
        elif m == 'REP':
            assert arg.startswith(('STOSD', 'MOVSD')), arg
            for _ in range(reg['ECX']):
                mem[reg['EDI']] = reg['EAX'] if arg.startswith('STOSD') else mem[reg['ESI']]
                reg['EDI'] += 4
                if arg.startswith('MOVSD'):
                    reg['ESI'] += 4
            reg['ECX'] = 0
        elif m == 'CALL':
            target = current_calls.get(pc)
            if target == 'round':
                reg['ESP'] -= 4;mem[reg['ESP']] = following
                frames.append((scope, following));scope='round';following=0
            else:
                assert target=='allocate' or read(arg)==0x900000, (scope,hex(pc),target,arg)
                if source:
                    assert mem[reg['ESP']+4]==0x900008
                size = mem[reg['ESP']]
                index = len(alloc_events)
                assert index<5
                alloc_events.append(size)
                reg.update(EAX=arrays[index], ECX=0xdddddd, EDX=0xeeeeee)
        elif m == 'FLD':
            fp.insert(0,read(arg,True,8 if 'QWORD' in arg or 'DOUBLE' in arg else 4))
        elif m in ('FST','FSTP'):
            width=8 if 'QWORD' in arg or 'DOUBLE' in arg else 4
            value=fp[0] if width==8 else f32(fp[0])
            st=re.fullmatch(r'ST\(?([0-7])?\)?',arg)
            if st:
                fp[int(st[1] or 0)]=fp[0]
            else:
                write(arg,value,width)
            if m=='FSTP':fp.pop(0)
        elif m == 'FXCH':
            if not arg:
                assert row['data']==bytes.fromhex('d9c9');parts=['ST1']
            st=re.fullmatch(r'ST\(?([0-7])?\)?',parts[-1]);assert st,(hex(pc),arg)
            index=int(st[1] or 0)
            fp[0],fp[index]=fp[index],fp[0]
        elif m in ('FADD','FSUB','FSUBR','FMUL'):
            index=0;left,right=fp[0],read(arg,True,8 if 'QWORD' in arg or 'DOUBLE' in arg else 4)
            if len(parts)==2:
                index=int(re.fullmatch(r'ST\(?([0-7])?\)?',parts[0])[1] or 0)
                left,right=fp[index],read(parts[1],True)
            if m=='FSUBR':left,right=right,left
            fp[index]=operation({'FADD':'add','FSUB':'sub','FSUBR':'sub','FMUL':'mul'}[m],left,right)
        elif m in ('FADDP','FMULP','FSUBP','FSUBRP'):
            assert row['data'][0]==0xde
            index=row['data'][1]&7
            left,right=fp[index],fp[0]
            if m=='FSUBRP':left,right=right,left
            fp[index]=operation('add' if m=='FADDP' else 'mul' if m=='FMULP' else 'sub',left,right);fp.pop(0)
        elif m == 'FSQRT':
            fp[0]=('sqrt53',fp[0])
        elif m == 'RET':
            ret=mem[reg['ESP']];reg['ESP']+=4
            if frames:
                scope,following=frames.pop();assert ret==following
            else:
                assert ret==-1 and not fp and reg['ESP']==4,(ret,fp,reg,hex(pc))
                assert all(reg[r]==initial[r] for r in ('EBX','EBP','ESI','EDI'))
                result=[]
                for base,size in zip(arrays,(faces*4,nodes*12,nodes*4,nodes*4,faces*12)):
                    if not lazy:result.append([mem[base+i] for i in range(0,size,4)])
                return dict(arrays=result,allocations=alloc_events,covered=visited,loops=loops,
                            nonstack_writes=writes,returned=reg['EAX'])
        else:
            raise AssertionError((scope,hex(pc),m,arg))
        pc=following
    raise AssertionError('Unbounded instruction walk')


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    obj=ROOT/'build/collision-full-original/bh_collision_cache.obj'
    symbols,sections=parse_coff(obj.read_bytes())
    symbol=next(s for s in symbols.values() if s['name']=='_bh_collision_cache_prepare')
    source,listing,end,_=owner_instructions(obj,symbols,sections,symbol['name'])
    section=sections[symbol['section']]
    original=[];old_calls={};old_constants={}
    image=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert digest(image)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    for r in json.loads((ROOT/'META/m4/collision-cache/original/0044b130.disasm.json').read_text()):
        va=int(r['address'],16);data=bytes.fromhex(r['bytes']);assert image_bytes(image,va,len(data))==data
        operands=', '.join(r['operands'])
        if r['mnemonic'].startswith('J'):operands=hex(int(operands,16)-0x44b130)
        if r['mnemonic']=='CALL':assert r['operands']==['0x0047dae0'];old_calls[va-0x44b130]='allocate'
        if '[0x00489e98]' in operands:old_constants[0x489e98]=constant(struct.unpack('<d',image_bytes(image,0x489e98,8))[0])
        original.append(dict(address=va-0x44b130,data=data,mnemonic=r['mnemonic'],operands=operands))
    assert b''.join(r['data'] for r in original)==image_bytes(image,0x44b130,423)
    new_constants={};new_calls={}
    for reloc in section['relocations']:
        row=next(r for r in source if r['address']<=reloc['offset']<r['address']+len(r['data']))
        sym=reloc['symbol']
        if reloc['kind']==20:
            new_calls[row['address']]={'_bh_round_float':'round','_bh_collision_cache_dispose':'dispose'}[sym['name']]
        else:
            assert reloc['kind']==6 and sym['section']>0
            at=sym['value']+struct.unpack_from('<I',section['data'],reloc['offset'])[0]
            raw=sections[sym['section']]['data'][at:at+8]
            assert row['data'][0]==0xdc and raw==struct.pack('<d',.5)
            assert '[0x0]' in row['operands']
            row['operands']=row['operands'].replace('[0x0]','[0x800000]')
            new_constants[0x800000]=constant(.5)
    helper=ROOT/'build/collision-full-original/bh_round.obj'
    hs,hsections=parse_coff(helper.read_bytes());hrows,hlisting,_,_=owner_instructions(helper,hs,hsections,'_bh_round_float')
    assert b''.join(r['data'] for r in hrows).hex()=='dd4424088b442404d918c3'
    proofs=[];ocover=set();ncover=set()
    for nf,nn in ((1,1),(2,1),(1,2),(3,3)):
        for explicit in (False,True):
            old=trace(original,old_constants,old_calls,{},False,nf,nn)
            new=trace(source,new_constants,new_calls,{'round':(hrows,{}, {})},True,nf,nn,explicit=explicit)
            assert old['arrays']==new['arrays'] and old['allocations']==new['allocations'],(nf,nn,explicit)
            assert new['returned']==1
            ocover|=old['covered'];ncover|=new['covered']
            proofs.append(dict(faces=nf,nodes=nn,explicit_allocator=explicit,outputs=old['arrays'],allocations=old['allocations'],original_loops=old['loops'],source_loops=new['loops']))
    for is_source,rows,constants,calls,helpers in ((False,original,old_constants,old_calls,{}),(True,source,new_constants,new_calls,{'round':(hrows,{}, {})})):
        result=trace(rows,constants,calls,helpers,is_source,1,1,lazy=True)
        assert not result['allocations'] and not result['nonstack_writes']
        if is_source:ncover|=result['covered']
        else:ocover|=result['covered']
    # Verify the actual loop tails used by the count induction, not C loop text.
    def tail(rows,start,stop):
        return ''.join(r['data'].hex() for r in rows if start<=r['address']<stop)
    assert tail(original,0x18b,0x19f)=='4383c13883c00cd95c9afc3b5e580f8c26ffffff'
    assert tail(source,0x2f3,0x319)=='8b5424188b44245883c40c4683c20c83c3383bf0897424208954240c895c24100f82cefeffff'
    assert tail(original,0xaf,0xb8)=='8b56503bca7cc9'
    assert tail(source,0x1b1,0x1cd)=='c74401fc000000008b442450c74490fc000000008b4424483bd072bf'
    # The signed original and unsigned source predicates agree on the admitted
    # decoded counts: 1<=N<=65535, 0<=k<N. All byte offsets remain below 2^31.
    assert 65535*56<2**31 and 65535*12<2**31
    # Independently exercise this analyzer's rejection of real operand/stride
    # changes. None of these edited rows is ever written to an object or EXE.
    controls=[]
    for label,at,old_text,new_text in (
            ('node stride',0x302,'0x38','0x3c'),
            ('squared radius operation',0x2b9,'',''),
            ('midpoint factor',0x221,'','')):
        changed=[dict(r) for r in source];changed_constants=dict(new_constants)
        row=next(r for r in changed if r['address']==at)
        if label=='node stride':
            assert old_text in row['operands'];row['operands']=row['operands'].replace(old_text,new_text)
        elif label=='squared radius operation':
            assert row['mnemonic']=='FADDP';row['mnemonic']='FMULP'
        else:
            changed_constants[0x800000]=constant(.25)
        rejected=False
        try:
            trial=trace(changed,changed_constants,new_calls,{'round':(hrows,{}, {})},True,3,3)
            reference=trace(original,old_constants,old_calls,{},False,3,3)
            rejected=trial['arrays']!=reference['arrays']
        except (AssertionError,KeyError):
            rejected=True
        assert rejected,label
        controls.append(dict(mutation=label,rejected=True))
    positive=ROOT/'build/game/bh_geometry.obj'
    ps,psections=parse_coff(positive.read_bytes());p=next(s for s in ps.values() if s['name']=='_bh_geometry_vertex_core')
    assert psections[p['section']]['data'][:87]==image_bytes(image,0x44e240,87)
    result=dict(passed=True,static_only=True,original_bytes=423,source_bytes=end,object_sha256=digest(obj.read_bytes()),
                precision='PC53 nearest, masked finite intermediate arithmetic; f64 spills are identity; only individual add/mul operands commute',
                symbolic_cases=proofs,original_covered=sorted(v for scope,v in ocover if scope=='owner'),
                source_covered=sorted(v for scope,v in ncover if scope=='owner'),actual_round_helper=hlisting,negative_controls=controls,
                positive_control='Complete87B production vertex core equals immutable original',
                count_recurrence_review=dict(original_node_tail=tail(original,0x18b,0x19f),source_node_tail=tail(source,0x2f3,0x319),
                    node_index='k -> k+1',node_offset='56*k -> 56*(k+1)',center_offset='12*k -> 12*(k+1)',
                    face_offset='12*j -> 12*(j+1)',face_flags='flags[j]=0; copied face[j].runtime=0',
                    domain='1<=face_count,node_count<=65535; 0<=j<face_count; 0<=k<node_count',
                    justification='Complete actual tails verified above; no float-dependent branches; coordinate loop visits0,4,8. Element expressions and allocation ordering proven by instruction algebra; initial offsets0 and actual +1/+12/+56 tails establish induction.'),
                limits=['Allocation callback modeled as successful nonmutating allocation with arbitrary volatile-register clobber; argument/failure handling are source adapters',
                        'Count extension uses the explicit reviewed affine loop induction, not sampled count totals',
                        'No PC24/PC64, NaN payload, overflow or floating exception flag proof; source transaction/publication layout is an explicit successful nonobserving-allocator adapter'])
    (OUT/'graphs.json').write_text(json.dumps(result,indent=2)+'\n')
    (OUT/'source.asm.txt').write_text(listing)
    print('Cache original/source symbolic allocation/write/PC53 arithmetic paths agree')


if __name__=='__main__':
    main()
