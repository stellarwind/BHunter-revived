"""Static full route-release path/argument/store proof. Never executes either image."""
import csv
import json
from pathlib import Path
import re
import struct
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from compare_ai_guided import instructions,branches,digest
from compare_ai_center import image_bytes
sys.path.insert(0,str(ROOT/'analysis/astra-review'))
from targeted_probes import parse_coff


def path_events(rows,calls,source,has_sound,has_previous,has_next):
    def pointer(name,offset=0):return ('pointer',name,offset)
    actor,room=pointer('actor'),pointer('room')
    previous=pointer('previous') if has_previous else 0
    following=pointer('next') if has_next else 0
    handle=('value','handle!=invalid') if has_sound else 0xffffffff
    registers={'ESP':0,'ESI':'saved.esi','EDI':'saved.edi'}
    memory={4:actor,8:pointer('binding'),('binding',0):pointer('sound'),('binding',4):pointer('pool'),
            ('binding',8):pointer('room',0x128),('actor',0x104):handle,('actor',0x1a4):room,
            ('actor',0xf8):previous,('actor',0xf4):following,('room',0x128):previous or actor,
            ('previous',0xf4):actor,('next',0xf8):actor}
    by_address={r['address']:i for i,r in enumerate(rows)};cursor=0;zero=None;events=[]
    def address(operand):
        inner=re.search(r'\[(.*?)\]',operand)[1].strip()
        found=re.fullmatch(r'([A-Z]{3})(?:\s*\+\s*(0X[0-9A-F]+))?',inner);assert found,operand
        base=registers[found[1]];offset=int(found[2] or '0',16)
        return (base[1],base[2]+offset) if isinstance(base,tuple) and base[0]=='pointer' else base+offset
    def value(operand):
        if '[' in operand:return memory[address(operand)]
        if operand in registers:return registers[operand]
        return int(operand,16)&0xffffffff
    def write(operand,item):
        if '[' not in operand:registers[operand]=item;return
        at=address(operand);memory[at]=item
        if isinstance(at,tuple):events.append(('store',at,item))
    while cursor<len(rows):
        row=rows[cursor];cursor+=1;op,arg=row['mnemonic'],row['operands'].upper();parts=[p.strip() for p in arg.split(',')]
        if op=='MOV':write(parts[0],value(parts[1]))
        elif op=='PUSH':
            item=value(arg);registers['ESP']-=4;memory[registers['ESP']]=item
        elif op=='POP':registers[arg]=memory[registers['ESP']];registers['ESP']+=4
        elif op=='ADD':
            base=value(parts[0]);amount=value(parts[1])
            write(parts[0],pointer(base[1],base[2]+amount) if isinstance(base,tuple) and base[0]=='pointer' else base+amount)
        elif op=='CMP':zero=value(parts[0])==value(parts[1])
        elif op=='TEST':
            assert parts[0]==parts[1];zero=value(parts[0])==0
        elif op in ('JZ','JE','JMP'):
            if op=='JMP' or zero:cursor=by_address[int(arg,16)]
        elif op=='CALL':
            role=calls[row['address']];argc=2 if source else 1
            args=[memory[registers['ESP']+4*i] for i in range(argc)]
            if role=='sound':
                assert args==([pointer('sound'),handle] if source else [handle]);events.append(('sound_stop',pointer('sound'),handle))
            else:
                assert role=='pool' and args==([pointer('pool'),actor] if source else [actor]);events.append(('entity_release',pointer('pool'),actor))
            registers.update(EAX=('return',role),ECX='clobbered.ecx',EDX='clobbered.edx')
        elif op=='RET':
            assert registers['ESP']==0 and registers['ESI']=='saved.esi'
            if source:assert registers['EDI']=='saved.edi'
            assert events[-1]==('entity_release',pointer('pool'),actor)
            assert memory[('actor',0x104)]==handle
            assert memory[('actor',0xf4)]==following and memory[('actor',0xf8)]==previous
            return events
        else:raise AssertionError(('Unreviewed release instruction',row))
    raise AssertionError('No complete return')


def main():
    out=ROOT/'META/m4/playable/ai-byte-match/route-release-close';out.mkdir(exist_ok=True)
    path=ROOT/'build/game/bh_actor_route_lifetime.obj';obj=path.read_bytes();symbols,sections=parse_coff(obj)
    owner=next(s for s in symbols.values() if s['name']=='_bh_actor_route_release_core');section=sections[owner['section']]
    source,listing=instructions(path,owner['name']);end=max(r['address']+len(r['data']) for r in source if r['mnemonic']=='RET');source=[r for r in source if r['address']<end]
    assert b''.join(r['data'] for r in source)==section['data'][:end] and set(section['data'][end:])<={0x90}
    reference=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert digest(reference)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    original=[]
    for row in csv.DictReader((ROOT/'META/m4/target-steering/original/00435720.tsv').read_text().splitlines(),delimiter='\t'):
        va=int(row['address'],16);data=bytes.fromhex(row['bytes']);op,*operand=row['instruction'].split(' ',1);arg=operand[0] if operand else ''
        assert data==image_bytes(reference,va,len(data))
        if op.startswith('J'):arg=hex(int(arg,16)-0x435720)
        original.append(dict(address=va-0x435720,data=data,mnemonic=op,operands=arg))
    assert b''.join(r['data'] for r in original)==image_bytes(reference,0x435720,102)
    assert branches(source)==branches(original)
    exe_path=ROOT/'build/game/BHunter-source.exe';exe=exe_path.read_bytes();map_text=(ROOT/'build/game/BHunter-source.map').read_text()
    def address(name):
        found=re.findall(r'^\s+\w+:\w+\s+'+re.escape(name)+r'\s+([0-9a-fA-F]{8})\s',map_text,re.M);assert len(set(found))==1,name
        return int(found[0],16)
    va=address(owner['name']);linked=bytearray(image_bytes(exe,va,end));source_calls={};bindings=[]
    roles={'_bh_sound_stop':'sound','_bh_entity_release':'pool'}
    for r in section['relocations']:
        at,s=r['offset'],r['symbol'];assert r['kind']==20 and s['name'] in roles
        assert struct.unpack_from('<I',section['data'],at)[0]==0
        target=va+at+4+struct.unpack_from('<i',linked,at)[0];assert target==address(s['name'])
        source_calls[at-1]=roles[s['name']];bindings.append(dict(offset=at,symbol=s['name'],linked_va=hex(target),role=roles[s['name']]))
        linked[at:at+4]=section['data'][at:at+4]
    assert linked==section['data'][:end] and list(source_calls.values())==['sound','pool']
    original_calls={r['address']:({0x47c140:'sound',0x477070:'pool'}[0x435720+r['address']+5+struct.unpack_from('<i',r['data'],1)[0]]) for r in original if r['mnemonic']=='CALL'}
    proofs=[]
    for sound in (0,1):
        for previous in (0,1):
            for following in (0,1):
                old=path_events(original,original_calls,False,sound,previous,following)
                new=path_events(source,source_calls,True,sound,previous,following)
                assert old==new,('Store/call/order differs',sound,previous,following,old,new)
                proofs.append(dict(has_sound=sound,has_previous=previous,has_next=following,events=new))
    wrapper=next(s for s in symbols.values() if s['name']=='_bh_actor_route_release');ws=sections[wrapper['section']];wrapper_va=address(wrapper['name']);code=bytearray(image_bytes(exe,wrapper_va,len(ws['data'])));wrapper_calls=[]
    for r in ws['relocations']:
        at,s=r['offset'],r['symbol'];assert r['kind']==20
        target=wrapper_va+at+4+struct.unpack_from('<i',code,at)[0]
        if s['storage']==3:
            definition=sections[s['section']];assert not definition['relocations'];body=definition['data'][s['value']:];assert image_bytes(exe,target,len(body))==body
        else:assert target==address(s['name'])
        wrapper_calls.append(s['name']);code[at:at+4]=ws['data'][at:at+4]
    assert code==ws['data'] and owner['name'] in wrapper_calls
    result=dict(original_va='00435720',original_body_bytes=102,source_body_bytes=end,byte_match=False,
                classification='func/close with explicit source queue/pool/head ABI',linked_matches_compared_object=True,actual_wrapper_calls_core=True,
                decision_edges=branches(source),all_eight_sound_list_paths=proofs,real_callee_bindings=bindings,
                differences='Queue/pool context arguments and direct borrowed room-head reference replace original globals/room layout; register/stack allocation differs. All field stores, retained payload and call order agree.',
                limits=['Not a binary drop-in ABI; checked source wrapper supplies real stable queue/pool/room-head owners.',
                        'Reference sound and entity release calls use real shared source primitives; those primitive implementations are not independently original-executed here.',
                        'Proof is of complete integer caller behavior on valid room-list topology; source invalid-input guards are separate.'],
                object_sha256=digest(obj),executable_sha256=digest(exe))
    (out/'linked-close.json').write_text(json.dumps(result,indent=2)+'\n');(out/'source.asm').write_text(listing)
    print(json.dumps({k:v for k,v in result.items() if k not in ('decision_edges','all_eight_sound_list_paths','real_callee_bindings')}))


if __name__=='__main__':main()
