"""Static complete pose instruction graphs and dependency argument traces; no execution.

Masked PC53 only: binary64 scratch spills are identity operations in the
float-input range. Commutative operands compare by value; no NaN payload or
exception-flag claim. All instructions, calls, branches and memory are reviewed.
"""
import argparse
import json
from pathlib import Path
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from compare_ai_guided import instructions as base_instructions, digest
from compare_ai_center import image_bytes
sys.path.insert(0, str(ROOT / 'analysis/astra-review'))
from targeted_probes import parse_coff


def instructions(path, symbol):
    _, listing = base_instructions(path, symbol)
    rows = []
    for line in listing.splitlines():
        match = re.match(r'\s*([0-9a-f]+):\s+((?:[0-9a-f]{2}\s+)+)(\w+)\s*(.*)', line)
        if match:
            address, data, mnemonic, operands = match.groups()
            rows.append(dict(address=int(address,16),data=bytes.fromhex(data),mnemonic=mnemonic.upper(),
                             operands=operands.split(' <')[0].strip()))
    return rows, listing


def trace(rows, constants, calls, source, damage, glow, sounding):
    def ptr(name, offset=0): return ('ptr', name, offset)
    def f32(v):
        if isinstance(v,tuple):
            if v[0]=='f32':return v
            if v[0]=='input' and (v[1]=='fade' or '.' in v[1]):return v
            if v[0]=='constant':return constant(struct.unpack('<f',struct.pack('<f',float.fromhex(v[1])))[0])
        return ('f32',v)
    def constant(v): return ('constant', float(v).hex())
    def operation(op, a, b):
        if op in ('add', 'mul') and repr(a) > repr(b): a, b = b, a
        return (op + '53', a, b)
    def plus(v, offset):
        return ptr(v[1], v[2] + offset) if isinstance(v, tuple) and v[0] == 'ptr' else v + offset
    def cell(v): return v[2] if isinstance(v, tuple) and v[:2] == ('ptr', 'stack') else v
    actor, context = ptr('actor'), ptr('context')
    registers = dict(ESP=0, ESI='saved.esi', EDI='saved.edi', EBX='saved.ebx', EBP='saved.ebp')
    memory = ({4: context, 8: actor, 12: ('input', 'clock')} if source else {4: actor})
    fields = {0x104:'sound', 0x108:'frames', 0x10c:'sound-resource', 0x114:'model',
              0x19c:'fade', 0x1e4:'tick', 0x1e6:'duration'}
    for offset, name in fields.items(): memory[ptr('actor', offset)] = ('input', name)
    for offset, name in [(0x120,'start'), (0x144,'first'), (0x150,'second'), (0x12c,'end'), (0x15c,'offset')]:
        for i in range(3): memory[ptr('actor', offset+4*i)] = ('input', name+'.'+'xyz'[i])
    for offset, name in [(0,'queue'), (4,'sound-queue'), (8,'damage-resource'), (12,'glowing-model'),
                         (16,'glow-first'), (20,'glow-second'), (28,'animation-clock')]:
        memory[ptr('context', offset)] = ('input', name)
    globals_ = {0x4dfac4:'clock',0x496334:'damage-resource',0x4d1940:'glowing-model',
                0x4f0078:'glow-first',0x4f008c:'glow-second',0x4f556c:'animation-clock'}
    stack, events, comparisons, covered = [], [], [], set()
    by_address = {r['address']:i for i,r in enumerate(rows)}
    cursor = 0; predicate = None; rand_count = curve_count = matrix_count = 0
    def address(operand):
        inner = re.search(r'\[(.*?)\]', operand)[1].strip()
        if inner.startswith('0X'): return ('global', int(inner,16))
        m = re.fullmatch(r'([A-Z]{3})(?:\s*([+-])\s*(0X[0-9A-F]+))?', inner)
        assert m, ('Unknown address', operand)
        offset = int(m[3] or '0',16) * (-1 if m[2] == '-' else 1)
        return cell(plus(registers[m[1]], offset))
    def st(operand):
        m = re.fullmatch(r'ST\(?([0-7])?\)?', operand.strip())
        return int(m[1] or '0') if m else None
    def value(operand, row, floating=False):
        index = st(operand)
        if index is not None: return stack[index]
        if '[' in operand:
            if row['address'] in constants: return constants[row['address']]
            at = address(operand)
            if isinstance(at,tuple) and at[0] == 'global': result = ('input', globals_[at[1]])
            else: result = memory[at]
        elif operand in registers: result = registers[operand]
        elif operand == 'AX': result = registers['AX']
        else: result = int(operand,16) & 0xffffffff
        if floating and isinstance(result,int): return constant(struct.unpack('<f',struct.pack('<I',result))[0])
        return result
    def write(operand, item, row, floating=False):
        index = st(operand)
        if index is not None: stack[index]=item;return
        if '[' not in operand: registers[operand]=item;return
        at=address(operand)
        if floating and not ('DOUBLE' in operand or 'QWORD' in operand): item=f32(item)
        memory[at]=item
        if isinstance(at,tuple) and at[:2] == ('ptr','actor'): events.append(('actor-store',at[2],item))
    def vector(p, count=3): return tuple(memory[cell(plus(p,4*i))] for i in range(count))
    def push(item): registers['ESP']-=4;memory[registers['ESP']]=item
    def float_arg(v):
        if isinstance(v,int): return constant(struct.unpack('<f',struct.pack('<I',v))[0])
        return v
    while cursor < len(rows):
        row=rows[cursor];cursor+=1;covered.add(row['address'])
        op,arg=row['mnemonic'],row['operands'].upper();parts=[p.strip() for p in arg.split(',')]
        if op in ('SUB','ADD'):
            if parts[0]=='AX': write('AX',('word-sub',value('AX',row),value(parts[1],row)),row)
            else: write(parts[0],plus(value(parts[0],row),int(parts[1],16)*(1 if op=='ADD' else -1)),row)
        elif op=='MOV': write(parts[0],value(parts[1],row),row)
        elif op=='MOVSX': write(parts[0],('sx16',value(parts[1],row)),row)
        elif op=='LEA':
            at=address(parts[1]);write(parts[0],ptr('stack',at) if isinstance(at,int) else at,row)
        elif op=='PUSH': push(value(arg,row))
        elif op=='POP': registers[arg]=memory[registers['ESP']];registers['ESP']+=4
        elif op=='AND': write(parts[0],('and',value(parts[0],row),int(parts[1],16)),row)
        elif op in ('FLD','FILD'):
            v=value(arg,row,op=='FLD');stack.insert(0,('i2f',v) if op=='FILD' else v)
        elif op=='FXCH':
            index=st(arg) if arg else 1;stack[0],stack[index]=stack[index],stack[0]
        elif op in ('FST','FSTP'):
            write(arg,stack[0],row,True)
            if op=='FSTP':stack.pop(0)
        elif op in ('FADD','FMUL','FSUB','FSUBR','FDIV','FDIVR','FIDIV'):
            index=st(parts[0]) if len(parts)==2 else 0
            left,right=stack[index],value(parts[-1],row,op!='FIDIV')
            if op=='FIDIV':right=('i2f',right)
            if op in ('FSUBR','FDIVR'):left,right=right,left
            stack[index]=operation({'FADD':'add','FMUL':'mul','FSUB':'sub','FSUBR':'sub','FDIV':'div','FDIVR':'div','FIDIV':'div'}[op],left,right)
        elif op in ('FADDP','FMULP','FSUBP','FSUBRP','FDIVP','FDIVRP'):
            index=st(parts[0]) if arg else 1;left,right=stack[index],stack[0]
            if op in ('FSUBRP','FDIVRP'):left,right=right,left
            stack[index]=operation({'FADDP':'add','FMULP':'mul','FSUBP':'sub','FSUBRP':'sub','FDIVP':'div','FDIVRP':'div'}[op],left,right);stack.pop(0)
        elif op in ('FSQRT','FSIN'):stack[0]=(op.lower()+'53',stack[0])
        elif op=='FPATAN':stack[1]=('atan2',stack[1],stack[0]);stack.pop(0)
        elif op=='FCOMP':
            comparisons.append(('damage',stack.pop(0),value(arg,row,True)));predicate=not damage
        elif op=='FNSTSW':assert arg=='AX'
        elif op=='TEST':assert arg in ('AH, 0X1','AH,0X1')
        elif op=='CMP':
            a,b=value(parts[0],row),value(parts[1],row)
            if a==('input','model') and b==('input','glowing-model'):predicate=glow;comparisons.append(('glow',a,b))
            else:
                assert a==('input','sound') and b==0xffffffff,(a,b)
                predicate=not sounding;comparisons.append(('sound',a,b))
        elif op in ('JZ','JE','JNZ','JNE','JMP'):
            take=op=='JMP' or (predicate if op in ('JZ','JE') else not predicate)
            if take:cursor=by_address[int(arg,16)]
        elif op=='CALL':
            assert not stack,('Live x87 call stack',row,stack)
            role=calls[row['address']]
            def arguments(n):return [memory[registers['ESP']+4*i] for i in range(n)]
            if role=='curve':
                t,out,*controls=arguments(6);assert controls==[ptr('actor',o) for o in (0x120,0x144,0x150,0x12c)]
                events.append(('curve',float_arg(t),tuple(vector(v) for v in controls)))
                for i in range(3):memory[cell(plus(out,4*i))]=f32(('curve-output',curve_count,i,float_arg(t)))
                curve_count+=1
            elif role in ('model','billboard','request','update'):
                n={'model':5,'billboard':8,'request':5,'update':5}[role];args=arguments(n+int(source))
                if source:
                    queue=args.pop(0);assert queue==('input','sound-queue' if role in ('request','update') else 'queue')
                if role=='model':args=[args[0],args[1],vector(args[2]),vector(args[3]),float_arg(args[4])]
                elif role=='billboard':args=[args[0],args[1],vector(args[2]),float_arg(args[3]),float_arg(args[4]),float_arg(args[5]),args[6],float_arg(args[7])]
                else:args=[args[0],float_arg(args[1]),float_arg(args[2]),vector(args[3]),vector(args[4])]
                events.append((role,tuple(args)))
            elif role=='rand':events.append(('rand',rand_count));rand_count+=1
            else:
                assert role in ('translate','rotate-x','rotate-y','rotate-z','transform'),role
                args=arguments(4 if role=='translate' else 3 if role=='transform' else 2)
                if role=='translate':out=args[3];detail=(tuple(float_arg(v) for v in args[:3]),vector(out,12))
                elif role=='transform':out=args[2];detail=(vector(args[0],12),vector(args[1]))
                else:out=args[1];detail=(float_arg(args[0]),vector(out,12))
                events.append((role,detail))
                for i in range(3 if role=='transform' else 12):memory[cell(plus(out,4*i))]=f32(('math-output',matrix_count,i))
                matrix_count+=1
            registers.update(EAX=('rand-result',rand_count-1) if role=='rand' else ('sound-result',) if role=='request' else ('call-result',role),ECX='clobbered.ecx',EDX='clobbered.edx')
        elif op=='RET':
            assert registers['ESP']==0 and not stack
            assert all(registers[r]=='saved.'+r.lower() for r in ('ESI','EDI','EBX','EBP'))
            assert curve_count==3 and rand_count==3*damage and matrix_count==8*glow
            return events,comparisons,covered
        else:raise AssertionError(('Unreviewed instruction',row))
    raise AssertionError('No return')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--object',type=Path,default=ROOT/'build/game/bh_actor_vehicle_draw.obj')
    parser.add_argument('--output',type=Path,default=ROOT/'build/ai-pose-match/static-graphs.json')
    args=parser.parse_args()
    obj=args.object.read_bytes();symbols,sections=parse_coff(obj)
    owner=next(s for s in symbols.values() if s['name']=='_bh_actor_vehicle_pose_core');section=sections[owner['section']]
    source,listing=instructions(args.object,owner['name']);end=max(r['address']+len(r['data']) for r in source if r['mnemonic']=='RET');source=[r for r in source if r['address']<end]
    assert b''.join(r['data'] for r in source)==section['data'][:end] and set(section['data'][end:])<={0x90}
    image=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert digest(image)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    original=[]
    for row in json.loads((ROOT/'META/m4/target600/b/original/00434920.disasm.json').read_text()):
        va=int(row['address'],16);data=bytes.fromhex(row['bytes']);assert data==image_bytes(image,va,len(data))
        operands=', '.join(row['operands'])
        if row['mnemonic'].startswith('J'):operands=hex(int(operands,16)-0x434920)
        original.append(dict(address=va-0x434920,data=data,mnemonic=row['mnemonic'],operands=operands))
    assert b''.join(r['data'] for r in original)==image_bytes(image,0x434920,1561)
    roles={'_bh_camera_curve':'curve','_bh_draw_submit_resource':'model','_bh_draw_billboard':'billboard','_rand':'rand',
           '_bh_mat34_translate_world':'translate','_bh_mat34_rotate_x':'rotate-x','_bh_mat34_rotate_y':'rotate-y',
           '_bh_mat34_rotate_z':'rotate-z','_bh_mat34_transform_point':'transform','_bh_sound_request':'request','_bh_sound_update':'update'}
    old_roles={0x4341a0:'curve',0x478170:'model',0x478280:'billboard',0x47ddf0:'rand',0x401460:'translate',
               0x401610:'rotate-x',0x4017e0:'rotate-y',0x4019a0:'rotate-z',0x4012e0:'transform',0x47bd40:'request',0x47c050:'update'}
    def fconstant(data):return ('constant',struct.unpack('<d' if len(data)==8 else '<f',data)[0].hex())
    source_constants={};source_calls={};relocations={r['offset']:r for r in section['relocations']}
    for row in source:
        offsets=[i for i in relocations if row['address']<=i<row['address']+len(row['data'])]
        if not offsets:continue
        assert len(offsets)==1
        offset=offsets[0];r=relocations[offset];s=r['symbol']
        if row['mnemonic']=='CALL':assert r['kind']==20;source_calls[row['address']]=roles[s['name']]
        else:
            assert r['kind']==6 and row['mnemonic'].startswith('F') and s['section']>0
            size=8 if 'QWORD' in row['operands'].upper() else 4
            at=s['value']+struct.unpack_from('<I',section['data'],offset)[0]
            source_constants[row['address']]=fconstant(sections[s['section']]['data'][at:at+size])
    original_constants={};original_calls={}
    for row in original:
        if row['mnemonic']=='CALL':original_calls[row['address']]=old_roles[0x434920+row['address']+5+struct.unpack_from('<i',row['data'],1)[0]]
        elif row['mnemonic'].startswith('F'):
            m=re.search(r'\[0x([0-9a-f]+)\]',row['operands'])
            if m:
                va=int(m[1],16)
                if va==0x4f556c:assert row['mnemonic']=='FILD'
                else:
                    assert 0x489000<=va<0x48b4e8
                    original_constants[row['address']]=fconstant(image_bytes(image,va,8 if 'double' in row['operands'] else 4))
    assert list(source_calls.values())==list(original_calls.values())
    proofs=[];old_covered=set();new_covered=set()
    for bits in range(8):
        inputs=[bool(bits&(1<<i)) for i in range(3)]
        old,oc,ov=trace(original,original_constants,original_calls,False,*inputs)
        new,nc,nv=trace(source,source_constants,source_calls,True,*inputs)
        if old!=new or oc!=nc:
            args.output.with_suffix('.difference.json').write_text(json.dumps(dict(inputs=inputs,original=old,source=new,original_comparisons=oc,source_comparisons=nc),indent=2)+'\n')
            raise AssertionError('Complete pose argument/store/rounding graph differs; see difference JSON')
        old_covered|=ov;new_covered|=nv;proofs.append(dict(damage_glow_sounding=inputs,events=old,comparisons=oc))
    assert old_covered=={r['address'] for r in original}
    assert new_covered=={r['address'] for r in source}
    linked_proof=None
    if args.object.resolve()==(ROOT/'build/game/bh_actor_vehicle_draw.obj').resolve():
        executable=ROOT/'build/game/BHunter-source.exe';linked_image=executable.read_bytes()
        map_text=(ROOT/'build/game/BHunter-source.map').read_text()
        def linked_address(name):
            found=re.findall(r'^\s+\w+:\w+\s+'+re.escape(name)+r'\s+([0-9a-fA-F]{8})\s',map_text,re.M)
            assert len(found)==1,('Non-unique linked symbol',name)
            return int(found[0],16)
        bindings=[]
        def verify_linked(symbol, body_rows, length):
            definition=sections[symbol['section']];va=linked_address(symbol['name'])
            normalized=bytearray(image_bytes(linked_image,va,length))
            for r in definition['relocations']:
                offset,s=r['offset'],r['symbol'];assert offset+4<=length
                if r['kind']==20:
                    assert normalized[offset-1]==0xe8
                    target=va+offset+4+struct.unpack_from('<i',normalized,offset)[0]
                    assert target==linked_address(s['name'])
                    bindings.append(dict(owner=symbol['name'],kind='real callee',symbol=s['name'],linked_va=hex(target)))
                else:
                    assert r['kind']==6
                    row=next(v for v in body_rows if v['address']<=offset<v['address']+len(v['data']))
                    assert row['address'] in source_constants
                    width=8 if 'QWORD' in row['operands'].upper() else 4
                    at=s['value']+struct.unpack_from('<I',definition['data'],offset)[0]
                    expected=sections[s['section']]['data'][at:at+width]
                    target=struct.unpack_from('<I',normalized,offset)[0]
                    assert image_bytes(linked_image,target,width)==expected
                    bindings.append(dict(owner=symbol['name'],kind='initialized constant',constant_hex=expected.hex(),linked_va=hex(target)))
                normalized[offset:offset+4]=definition['data'][offset:offset+4]
            assert normalized==definition['data'][:length],('Linked body changed',symbol['name'])
        verify_linked(owner,source,end)
        wrapper=next(s for s in symbols.values() if s['name']=='_bh_actor_vehicle_draw')
        wrapper_rows,wrapper_listing=instructions(args.object,wrapper['name'])
        wrapper_end=max(r['address']+len(r['data']) for r in wrapper_rows if r['mnemonic']=='RET')
        w=sections[wrapper['section']]
        assert [r['symbol']['name'] for r in w['relocations']]==['_bh_math_environment_supported','_bh_actor_route_now','_bh_actor_vehicle_pose_core']
        assert w['data'][0x27:0x31].hex()=='6683bfe6010000007437' # duration word guard
        assert w['data'][0x31:0x4e].hex()=='8b462485c0741050e80000000083c40425ffff0000eb0633c0668b4618' # bound clock or snapshot
        assert w['data'][0x4e:0x59].hex()=='505756e80000000083c40c' # tick, actor, context
        assert w['data'][0x59:0x68].hex()=='c7462000000000b8010000005f5ec3' # error=OK, return1
        verify_linked(wrapper,wrapper_rows,wrapper_end)
        linked_proof=dict(executable=str(executable.relative_to(ROOT)),executable_sha256=digest(linked_image),
                          core_and_wrapper_equal_compared_objects=True,actual_wrapper_calls_core=True,
                          wrapper_duration_guard_and_live_clock_arguments_verified=True,bindings=bindings,wrapper_listing=wrapper_listing)
    result=dict(original_va='00434920',original_body_bytes=1561,source_body_bytes=end,byte_match=False,
                profile='PC53 nearest/masked finite float-input arithmetic; commutative operands compare by value',
                all_eight_paths_equal=True,all_instructions_covered=True,proofs=proofs,
                source_object=str(args.object),object_sha256=digest(obj),source_listing=listing,linked=linked_proof,
                limits=['Binary64 scratch spills are identities only under the declared PC53/range profile.',
                        'Calls are compared by complete arguments and observable store order, not callee internals.',
                        'No NaN payload, exception flags, PC24/64 or original drop-in ABI claim.'])
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k not in ('proofs','source_listing','linked')} | dict(linked_production_verified=linked_proof is not None)))


if __name__=='__main__':main()
