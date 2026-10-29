"""Static original/source promotion list prefix and complete linked-body audit.

No code is executed. The remainder's FP/decision graphs are explicitly unproved;
this never labels the complete owner matched from its list-prefix evidence.
"""
import csv
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
sys.path.insert(0,str(ROOT/'analysis/astra-review'))
from targeted_probes import parse_coff


def prefix_trace(rows,source,previous,next_,new_head,same):
    def ptr(name,offset=0):return ('pointer',name,offset)
    old='room.shared' if same else 'room.old';new=old if same else 'room.new'
    actor=ptr('actor');binding=ptr('binding');prev=ptr('previous') if previous else 0;nxt=ptr('next') if next_ else 0
    head=prev if previous else actor;destination=head if same else ptr('destination') if new_head else 0
    memory={4:actor,8:binding if source else 0,12:0,16:1,
            ('actor',0xf8):prev,('actor',0xf4):nxt,('actor',0x1a4):ptr(old),('actor',0x1a8):ptr(new),
            (old,0x128):head,(new,0x128):destination,
            ('binding',4):ptr('record.old'),('binding',8):ptr('record.new'),('binding',12):ptr(new),
            ('binding',16):ptr('incoming'),('binding',20):ptr('outgoing')}
    for i in range(3):memory[('actor',0x120+4*i)]=('start.before',i);memory[('actor',0x12c+4*i)]=('end',i)
    regs=dict(ESP=0,ESI='saved.esi',EDI='saved.edi',EBX='saved.ebx',EBP='saved.ebp')
    events=[];cursor=0;zero=None;lookup={r['address']:i for i,r in enumerate(rows)};covered=set()
    def address(operand):
        inner=re.search(r'\[(.*?)\]',operand)[1].strip();m=re.fullmatch(r'([A-Z]{3})(?:\s*\+\s*(0X[0-9A-F]+))?',inner);assert m,operand
        base=regs[m[1]];offset=int(m[2] or '0',16)
        if isinstance(base,tuple) and base[0]=='pointer':
            name,offset=base[1],base[2]+offset
            if name in ('record.old','record.new') and offset==4:return (old if name=='record.old' else new,0x128)
            return (name,offset)
        return base+offset
    def value(operand):
        if '[' in operand:return memory[address(operand)]
        return regs[operand] if operand in regs else int(operand,16)
    def write(operand,item):
        if '[' not in operand:regs[operand]=item;return
        at=address(operand);memory[at]=item
        if isinstance(at,tuple):events.append(('store',at,item))
    while cursor<len(rows):
        row=rows[cursor];cursor+=1;covered.add(row['address']);op,arg=row['mnemonic'],row['operands'].upper();parts=[p.strip() for p in arg.split(',')]
        if op.startswith('F'):return events,covered-{row['address']}
        if op=='MOV':write(parts[0],value(parts[1]))
        elif op=='LEA':
            at=address(parts[1]);write(parts[0],ptr(*at) if isinstance(at,tuple) else at)
        elif op=='PUSH':item=value(arg);regs['ESP']-=4;memory[regs['ESP']]=item
        elif op in ('ADD','SUB'):
            base=value(parts[0]);amount=int(parts[1],16)*(1 if op=='ADD' else -1)
            write(parts[0],ptr(base[1],base[2]+amount) if isinstance(base,tuple) else base+amount)
        elif op=='TEST':assert parts[0]==parts[1];zero=value(parts[0])==0
        elif op in ('JZ','JE','JNZ','JNE','JMP'):
            take=op=='JMP' or (zero if op in ('JZ','JE') else not zero)
            if take:cursor=lookup[int(arg,16)]
        else:raise AssertionError(('Unknown prefix instruction',row))
    raise AssertionError('Missing arithmetic boundary')


def main():
    out=ROOT/'META/m4/playable/ai-byte-match/promotion-core';out.mkdir(exist_ok=True)
    path=ROOT/'build/game/bh_actor_route.obj';obj=path.read_bytes();symbols,sections=parse_coff(obj)
    functions={s['name']:s for s in symbols.values() if s['section']>0 and s['type']==32}
    owner=functions['_bh_actor_route_promote_core'];source,listing=instructions(path,owner['name'])
    end=max(r['address']+len(r['data']) for r in source if r['mnemonic']=='RET')
    source=[r for r in source if r['address']<end];assert b''.join(r['data'] for r in source)==sections[owner['section']]['data'][:end]
    original=[];image=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert digest(image)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    for row in csv.DictReader((ROOT/'META/m4/target-steering/original/00434f40.tsv').open(),delimiter='\t'):
        va=int(row['address'],16);data=bytes.fromhex(row['bytes']);assert data==image_bytes(image,va,len(data))
        op,_,arg=row['instruction'].partition(' ')
        if op.startswith('J') and '[' not in arg:arg=hex(int(arg,16)-0x434f40)
        original.append(dict(address=va-0x434f40,data=data,mnemonic=op,operands=arg))
    assert b''.join(r['data'] for r in original)==image_bytes(image,0x434f40,1600)
    proofs=[];old_covered=set();new_covered=set()
    for same in (False,True):
        for previous in (False,True):
            for next_ in (False,True):
                for head in ((False,) if same else (False,True)):
                    inputs=(previous,next_,head,same);old,oc=prefix_trace(original,False,*inputs);new,nc=prefix_trace(source,True,*inputs)
                    assert old==new,('Ordered list/ownership/start stores differ',inputs,old,new)
                    proofs.append(dict(previous_next_new_head_same_room=inputs,events=old));old_covered|=oc;new_covered|=nc
    def prefix(rows):return rows[:next(i for i,r in enumerate(rows) if r['mnemonic'].startswith('F'))]
    assert old_covered=={r['address'] for r in prefix(original)} and new_covered=={r['address'] for r in prefix(source)}
    exe=(ROOT/'build/game/BHunter-source.exe').read_bytes();map_text=(ROOT/'build/game/BHunter-source.map').read_text();verified=set();bindings=[]
    def address(name):
        found=re.findall(r'^\s+\w+:\w+\s+'+re.escape(name)+r'\s+([0-9a-fA-F]{8})\s',map_text,re.M);assert len(found)==1,name
        return int(found[0],16)
    def linked(symbol,va):
        if (symbol['name'],va) in verified:return
        verified.add((symbol['name'],va));rows,_=instructions(path,symbol['name']);length=max(r['address']+len(r['data']) for r in rows if r['mnemonic']=='RET')
        definition=sections[symbol['section']];code=bytearray(image_bytes(exe,va,length))
        for r in definition['relocations']:
            offset,s=r['offset'],r['symbol'];assert offset+4<=length
            if r['kind']==20:
                assert code[offset-1]==0xe8;target=va+offset+4+struct.unpack_from('<i',code,offset)[0]
                if s['storage']==3:linked(functions[s['name']],target)
                else:assert target==address(s['name'])
                bindings.append(dict(owner=symbol['name'],callee=s['name'],linked_va=hex(target)))
            else:
                assert r['kind']==6 and s['section']>0
                row=next(v for v in rows if v['address']<=offset<v['address']+len(v['data']));assert row['mnemonic'].startswith('F'),row
                width=8 if 'QWORD' in row['operands'].upper() else 4;at=s['value']+struct.unpack_from('<I',definition['data'],offset)[0]
                expected=sections[s['section']]['data'][at:at+width];assert image_bytes(exe,struct.unpack_from('<I',code,offset)[0],width)==expected
            code[offset:offset+4]=definition['data'][offset:offset+4]
        assert code==definition['data'][:length],('Linked implementation differs',symbol['name'])
    linked(owner,address(owner['name']));wrapper=functions['_bh_actor_route_promote'];assert any(r['symbol']['name']==owner['name'] for r in sections[wrapper['section']]['relocations']);linked(wrapper,address(wrapper['name']))
    result=dict(original_va='00434f40',original_body_bytes=1600,source_body_bytes=end,byte_match=False,whole_owner_cosmetic_only_proved=False,
                prefix_cases=12,all_prefix_instructions_covered=True,original_prefix_bytes=sum(len(r['data']) for r in prefix(original)),
                source_prefix_bytes=sum(len(r['data']) for r in prefix(source)),ordered_prefix_proofs=proofs,
                real_linked_core_wrapper_and_local_callees_verified=True,linked_bindings=bindings,object_sha256=digest(obj),executable_sha256=digest(exe),
                limits=['List/ownership/start-copy prefix only: not an independent original owner or whole-function match.',
                        'Remaining FP graphs, switch shapes and source numerical guards are NOT proved cosmetic-only.',
                        'Native functional evidence is recorded separately; totals cannot establish instruction fidelity.'])
    (out/'linked-prefix.json').write_text(json.dumps(result,indent=2)+'\n');(out/'source.asm').write_text(listing)
    print(json.dumps({k:v for k,v in result.items() if k not in ('ordered_prefix_proofs','linked_bindings')}))


if __name__=='__main__':main()
