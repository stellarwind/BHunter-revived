"""Static complete route-advance owner/linked ABI/RNG/x87 comparison; no execution."""
import csv
import json
from pathlib import Path
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
from compare_ai_guided import instructions, branches, digest
from compare_ai_center import image_bytes
sys.path.insert(0, str(ROOT/'analysis/astra-review'))
from targeted_probes import parse_coff


def guards(rows):
    result = []
    for i, row in enumerate(rows):
        if not row['mnemonic'].startswith('J') or row['mnemonic'] == 'JMP':
            continue
        previous = next(r for r in reversed(rows[:i]) if r['mnemonic'] in ('CMP','TEST'))
        operand = re.sub(r'\s+', '', previous['operands'].upper())
        if len(result) == 3:
            assert operand in ('ECX,ECX','EDX,EDX')
            operand = 'COUNT,COUNT'
        elif len(result) == 4:
            assert operand in ('DWORDPTR[EDX],EBP','DWORDPTR[ECX],EBP')
            operand = 'DWORDPTR[PORTAL],CURRENT'
        elif len(result) == 6:
            assert operand in ('EAX,ECX','EAX,EDX')
            operand = 'INDEX,COUNT'
        result.append((previous['mnemonic'],operand))
    return result


def update_graph(rows, constants):
    start = next(i for i,r in enumerate(rows) if r['mnemonic']=='MOV' and
                 re.sub(r'\s+','',r['operands'].upper())=='EDX,DWORDPTR[ESI+0X1E8]')
    regs = {'ESP':0,'ESI':('pointer','actor'),'EBP':('pointer','binding'),
            'EDI':('value','incoming'),'EAX':('value','outgoing')}
    memory = {('actor',0x1e8):'rng',('actor',0x18c):'u',('actor',0x190):'v',
              ('actor',0x1b0):'jitter',('binding',0):('pointer','context'),
              0x20:('pointer','binding'),0x1c:('pointer','actor')}
    stack, writes = [], []
    def address(operand):
        inner = re.search(r'\[(.*?)\]',operand)[1].strip()
        found = re.fullmatch(r'([A-Z]{3})(?:\s*\+\s*(0X[0-9A-F]+))?',inner)
        assert found,operand
        base = regs[found[1]]; offset = int(found[2] or '0',16)
        return (base[1],offset) if isinstance(base,tuple) and base[0]=='pointer' else base+offset
    def st_index(operand):
        found = re.fullmatch(r'ST\(?([0-7])?\)?',operand.strip())
        return int(found[1] or '0') if found else None
    def value(operand,row):
        index = st_index(operand)
        if index is not None:return stack[index]
        if '[' in operand:
            if row['address'] in constants:return ('constant',constants[row['address']])
            return memory[address(operand)]
        if operand in regs:return regs[operand]
        return int(operand,16)&0xffffffff
    def operation(op,a,b):
        if op in ('add53','mul53','mul32','xor32','or32','and32') and repr(a)>repr(b):a,b=b,a
        if op=='xor32' and a==b:return 0
        return (op,a,b)
    def write(operand,item):
        if '[' not in operand:regs[operand]=item;return
        at=address(operand);memory[at]=item
        if isinstance(at,tuple) and at[0]=='actor':writes.append((at[1],item))
    for row in rows[start:]:
        op,arg=row['mnemonic'],row['operands'].upper();parts=[p.strip() for p in arg.split(',')]
        if op=='MOV':write(parts[0],value(parts[1],row))
        elif op in ('IMUL','SHR','OR','XOR','AND'):
            left=value(parts[1],row) if op=='IMUL' else value(parts[0],row)
            right=value(parts[2],row) if op=='IMUL' else value(parts[1],row)
            write(parts[0],operation({'IMUL':'mul32','SHR':'shr32','OR':'or32','XOR':'xor32','AND':'and32'}[op],left,right))
        elif op=='PUSH':
            item=value(arg,row);regs['ESP']-=4;memory[regs['ESP']]=item
        elif op=='ADD':
            assert parts[0]=='ESP';regs['ESP']+=int(parts[1],16)
        elif op=='FLD':stack.insert(0,value(arg,row))
        elif op=='FILD':
            assert 'QWORD' in arg
            at=address(arg);assert memory[at+4]==0
            stack.insert(0,('unsigned_integer_to_fp',memory[at]))
        elif op=='FXCH':
            index=st_index(arg) if arg else 1;stack[0],stack[index]=stack[index],stack[0]
        elif op in ('FMUL','FADD','FSUB','FSUBR'):
            index=st_index(parts[0]) if len(parts)==2 else 0
            a,b=stack[index],value(parts[-1],row)
            if op=='FSUBR':a,b=b,a
            stack[index]=operation({'FMUL':'mul53','FADD':'add53','FSUB':'sub53','FSUBR':'sub53'}[op],a,b)
        elif op in ('FSUBP','FADDP','FMULP'):
            index=st_index(parts[0]) if arg else 1
            stack[index]=operation({'FSUBP':'sub53','FADDP':'add53','FMULP':'mul53'}[op],stack[index],stack[0]);stack.pop(0)
        elif op=='FSTP':
            index=st_index(arg)
            if index is not None:stack[index]=stack[0]
            else:write(arg,('f32',stack[0]))
            stack.pop(0)
        elif op=='CALL':
            assert not stack
            args=[memory[i] for i in range(regs['ESP'],0,4)]
            assert args[-3:]==[('pointer','actor'),('value','incoming'),('value','outgoing')]
            assert len(args) in (3,4)
            if len(args)==4:assert args[0]==('pointer','context')
            return dict(writes=writes,promotion_arguments=args)
        else:raise AssertionError(('Unreviewed update instruction',row))
    raise AssertionError('Missing promotion')


def main():
    out=ROOT/'META/m4/playable/ai-byte-match/advance-close';out.mkdir(exist_ok=True)
    path=ROOT/'build/game/bh_actor_route.obj';obj=path.read_bytes();symbols,sections=parse_coff(obj)
    owner=next(s for s in symbols.values() if s['name']=='_bh_actor_route_advance_core');section=sections[owner['section']]
    source,listing=instructions(path,owner['name']);end=max(r['address']+len(r['data']) for r in source if r['mnemonic']=='RET');source=[r for r in source if r['address']<end]
    assert b''.join(r['data'] for r in source)==section['data'][:end] and set(section['data'][end:])<={0x90}
    original=[];reference=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert digest(reference)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    for row in csv.DictReader((ROOT/'META/m4/target-steering/original/004355c0.tsv').read_text().splitlines(),delimiter='\t'):
        va=int(row['address'],16);data=bytes.fromhex(row['bytes']);op,*operand=row['instruction'].split(' ',1);arg=operand[0] if operand else ''
        assert data==image_bytes(reference,va,len(data))
        if op.startswith('J'):arg=hex(int(arg,16)-0x4355c0)
        original.append(dict(address=va-0x4355c0,data=data,mnemonic=op,operands=arg))
    assert b''.join(r['data'] for r in original)==image_bytes(reference,0x4355c0,350)
    assert branches(source)==branches(original)
    assert guards(source)==guards(original)
    exe_path=ROOT/'build/game/BHunter-source.exe';exe=exe_path.read_bytes();map_text=(ROOT/'build/game/BHunter-source.map').read_text()
    def address(name,local=False):
        suffix=r'[^\n]*bh_actor_route\.obj' if local else ''
        found=re.findall(r'^\s+\w+:\w+\s+'+re.escape(name)+r'\s+([0-9a-fA-F]{8})\s'+suffix,map_text,re.M);assert len(set(found))==1,name
        return int(found[0],16)
    linked_va=address(owner['name']);linked=bytearray(image_bytes(exe,linked_va,end));constants={};bindings=[];calls=[]
    expected_calls={'_bh_sound_stop':0x47c140,'_bh_actor_guided_exit_core':0x434650,'_bh_actor_choose_exit_core':0x4344f0,'_bh_actor_route_promote':0x434f40}
    for r in section['relocations']:
        offset,symbol=r['offset'],r['symbol'];target=struct.unpack_from('<I',linked,offset)[0];addend=struct.unpack_from('<I',section['data'],offset)[0]
        if r['kind']==20:
            assert symbol['name'] in expected_calls and addend==0
            target=linked_va+offset+4+struct.unpack_from('<i',linked,offset)[0];assert target==address(symbol['name']);calls.append(expected_calls[symbol['name']]);kind='verified real source callee with explicit context ABI where required'
        elif symbol['name']=='_bh_actor_exit_counts':
            assert r['kind']==6 and addend in (0,0x18) and target==address(symbol['name'])+addend
            table=image_bytes(exe,address(symbol['name']),7*0x174);assert [struct.unpack_from('<i',table,i*0x174+0x18)[0] for i in range(7)]==list(range(7));kind='existing count image adapter'
        else:
            assert r['kind']==6 and symbol['section']>0
            opcode,modrm=section['data'][offset-2:offset];assert opcode==0xdc and modrm&0xc7==5
            data=sections[symbol['section']]['data'][symbol['value']+addend:symbol['value']+addend+8];number=struct.unpack('<d',data)[0];assert number in (1.,-1./32768.)
            assert image_bytes(exe,target,8)==data;constants[offset-2]=number;kind='verified original-valued x87 double constant'
        bindings.append(dict(offset=offset,symbol=symbol['name'],kind=kind));linked[offset:offset+4]=section['data'][offset:offset+4]
    assert linked==section['data'][:end]
    old_calls=[0x4355c0+r['address']+5+struct.unpack_from('<i',r['data'],1)[0] for r in original if r['mnemonic']=='CALL']
    assert calls==old_calls==[0x47c140,0x434650,0x4344f0,0x434f40]
    old_constants={r['address']:struct.unpack('<d',image_bytes(reference,int(re.search(r'0x([0-9a-fA-F]+)',r['operands'])[1],16),8))[0] for r in original if r['mnemonic'] in ('FSUBR','FMUL') and 'double ptr [0x' in r['operands']}
    old,new=update_graph(original,old_constants),update_graph(source,constants)
    assert old['writes']==new['writes'],('RNG, floating update graph or store order differs',old,new)
    assert [at for at,_ in new['writes']]==[0x18c,0x1e8,0x190]
    wrapper=next(s for s in symbols.values() if s['name']=='_bh_actor_route_advance');ws=sections[wrapper['section']];va=address(wrapper['name']);code=bytearray(image_bytes(exe,va,len(ws['data'])));wrapper_calls=[]
    for r in ws['relocations']:
        at,s=r['offset'],r['symbol'];assert r['kind']==20
        target=va+at+4+struct.unpack_from('<i',code,at)[0]
        if s['storage']==3:
            definition=sections[s['section']];assert not definition['relocations']
            body=definition['data'][s['value']:]
            assert image_bytes(exe,target,len(body))==body, 'Folded local helper differs from real definition'
        else:assert target==address(s['name'])
        wrapper_calls.append(s['name']);code[at:at+4]=ws['data'][at:at+4]
    assert code==ws['data'] and owner['name'] in wrapper_calls
    result=dict(original_va='004355c0',original_body_bytes=350,source_body_bytes=end,byte_match=False,
                classification='func/close with explicit source context/image ABI, PC53',linked_matches_compared_object=True,
                actual_wrapper_calls_core=True,decision_edges=branches(source),guard_templates=guards(source),bindings=bindings,
                rng_and_bias_writes=new['writes'],original_promotion_arguments=old['promotion_arguments'],source_promotion_arguments=new['promotion_arguments'],
                differences='Explicit context/current-room binding and call arguments, register allocation, scheduling, equivalent indexed count loads; RNG/update expressions and U/RNG/V publication order agree.',
                limits=['Source ABI is not a binary drop-in: source contexts and borrowed room images replace original globals/layout.',
                        'Masked PC53 nearest floating value graph; exception flags and runtime PC24/64 transitions unproved.',
                        'Wrapper guards reject invalid original room/index domains; original code is never run on those inputs.'],object_sha256=digest(obj),executable_sha256=digest(exe))
    (out/'linked-close.json').write_text(json.dumps(result,indent=2)+'\n');(out/'source.asm').write_text(listing)
    print(json.dumps({k:v for k,v in result.items() if k not in ('decision_edges','guard_templates','bindings','rng_and_bias_writes')}))


if __name__=='__main__':main()
