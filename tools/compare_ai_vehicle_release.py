"""Static complete vehicle-release caller/adapter proof. No executable is launched."""
import json
from pathlib import Path
import re
import struct
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from compare_ai_guided import instructions,digest
from compare_ai_center import image_bytes
sys.path.insert(0,str(ROOT/'analysis/astra-review'))
from targeted_probes import parse_coff


def release_trace(rows,calls,source,sound,rprev,rnext,gprev,gnext,moved):
    def ptr(name,offset=0):return ('pointer',name,offset)
    actor,context,pool,queue=ptr('actor'),ptr('context'),ptr('pool'),ptr('sound')
    world='room.B' if moved else 'room.A'
    handle=('value','handle!=invalid') if sound else 0xffffffff
    regs={'ESP':0,'ESI':'saved.esi','EDI':'saved.edi'}
    memory={4:actor,8:context,('context',0):pool,('context',4):ptr('route'),
            ('route',12):queue,('context',8):'head.before',('context',12):'count.before',
            ('actor',0x104):handle,('actor',0x1a4):ptr('world.room.A'),
            ('actor',0xf8):ptr('old.rprev'),('actor',0xf4):ptr('old.rnext'),
            ('actor',0xec):ptr('old.gprev'),('actor',0xf0):ptr('old.gnext')}
    events=[];cursor=0;flags=None;lookup={r['address']:i for i,r in enumerate(rows)}
    def address(operand):
        inner=re.search(r'\[(.*?)\]',operand)[1].strip()
        if inner=='0X004DFB28':return ('context',8)
        if inner=='0X004DFB2C':return ('context',12)
        match=re.fullmatch(r'([A-Z]{3})(?:\s*\+\s*(0X[0-9A-F]+))?',inner);assert match,operand
        base=regs[match[1]];offset=int(match[2] or '0',16)
        if isinstance(base,tuple) and base[0]=='pointer':
            offset+=base[2];name=base[1]
            if name.startswith('world.') and offset==0x128:return (name[6:],'head')
            if name.startswith('record.') and offset==4:return (name[7:],'head')
            return (name,offset)
        return base+offset
    def value(operand):
        if '[' in operand:return memory[address(operand)]
        if operand in regs:return regs[operand]
        return int(operand,16)&0xffffffff
    def write(operand,item):
        if '[' not in operand:regs[operand]=item;return
        at=address(operand);memory[at]=item
        if isinstance(at,tuple):events.append(('store',at,item))
    while cursor<len(rows):
        row=rows[cursor];cursor+=1;op,arg=row['mnemonic'],row['operands'].upper();parts=[p.strip() for p in arg.split(',')]
        if op=='MOV':write(parts[0],value(parts[1]))
        elif op=='LEA':write(parts[0],('stack',address(parts[1])))
        elif op=='PUSH':
            item=value(arg);regs['ESP']-=4;memory[regs['ESP']]=item
        elif op=='POP':regs[arg]=memory[regs['ESP']];regs['ESP']+=4
        elif op in ('SUB','ADD'):
            base=value(parts[0]);amount=value(parts[1])*(1 if op=='ADD' else -1)
            write(parts[0],ptr(base[1],base[2]+amount) if isinstance(base,tuple) and base[0]=='pointer' else base+amount)
        elif op=='DEC':write(arg,('dec32',value(arg)))
        elif op=='CMP':flags=value(parts[0])==value(parts[1])
        elif op=='TEST':assert parts[0]==parts[1];flags=value(parts[0])==0
        elif op in ('JE','JZ','JNE','JNZ','JMP'):
            take=op=='JMP' or (not flags if op in ('JNE','JNZ') else flags)
            if take:cursor=lookup[int(arg,16)]
        elif op=='CALL':
            role=calls[row['address']]
            argc=(2 if source else 1) if role!='notify' else (3 if source else 5)
            args=[memory[regs['ESP']+4*i] for i in range(argc)]
            if role=='sound':
                assert args==([queue,handle] if source else [handle]);events.append(('sound_stop',queue,handle));result=1
            elif role=='notify':
                if source:assert args[:2]==[context,actor]
                else:assert args[:4]==[0,0,0,0]
                packet=args[-1];assert packet[0]=='stack' and memory[packet[1]]==17 and memory[packet[1]+4]==actor
                assert not any(e[0]=='store' for e in events)
                events.append(('notify',pool,17,actor))
                # Independent post-callback inputs prevent a cached pre-query
                # room, count or neighbor value from masquerading as a reload.
                memory[('actor',0x1a4)]=ptr('world.'+world)
                for offset,name,present in [(0xf8,'rprev',rprev),(0xf4,'rnext',rnext),(0xec,'gprev',gprev),(0xf0,'gnext',gnext)]:memory[('actor',offset)]=ptr(name) if present else 0
                memory[('context',12)]='count.post';memory[('context',8)]='head.post'
                result=ptr('record.'+world) if source else 'query.return'
            else:
                assert role=='pool' and args==([pool,actor] if source else [actor]);events.append(('entity_release',pool,actor));result=1
            regs.update(EAX=result,ECX='clobbered.ecx',EDX='clobbered.edx')
        elif op=='RET':
            assert regs['ESP']==0 and regs['ESI']=='saved.esi'
            if source:assert regs['EDI']=='saved.edi' and regs['EAX']==1
            assert events[-1]==('entity_release',pool,actor) and memory[('actor',0x104)]==handle
            assert memory[('context',12)]==('dec32','count.post')
            return events
        else:raise AssertionError(('Unreviewed vehicle caller instruction',row))
    raise AssertionError('Missing complete return')


def main():
    out=ROOT/'META/m4/playable/ai-byte-match/vehicle-release-close';out.mkdir(exist_ok=True)
    path=ROOT/'build/game/bh_actor_vehicle_lifetime.obj';obj=path.read_bytes();symbols,sections=parse_coff(obj)
    functions={s['name']:s for s in symbols.values() if s['section']>0 and s['type']==32}
    owner=functions['_bh_actor_vehicle_release_core'];section=sections[owner['section']]
    source,listing=instructions(path,owner['name']);end=max(r['address']+len(r['data']) for r in source if r['mnemonic']=='RET');source=[r for r in source if r['address']<end]
    assert b''.join(r['data'] for r in source)==section['data'][:end] and set(section['data'][end:])<={0x90}
    reference=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert digest(reference)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    original=[]
    for row in json.loads((ROOT/'META/m4/target600/b2/original/004397e0.disasm.json').read_text()):
        va=int(row['address'],16)
        if va>=0x4398b4:continue
        data=bytes.fromhex(row['bytes']);assert data==image_bytes(reference,va,len(data));arg=', '.join(row['operands'])
        if row['mnemonic'].startswith('J'):arg=hex(int(arg,16)-0x4397e0)
        original.append(dict(address=va-0x4397e0,data=data,mnemonic=row['mnemonic'],operands=arg))
    assert b''.join(r['data'] for r in original)==image_bytes(reference,0x4397e0,212)
    roles={'_bh_sound_stop':'sound','_vehicle_release_notice_room':'notify','_bh_entity_release':'pool'}
    source_calls={r['offset']-1:roles[r['symbol']['name']] for r in section['relocations']}
    old_roles={0x47c140:'sound',0x4770f0:'notify',0x477070:'pool'}
    original_calls={r['address']:old_roles[0x4397e0+r['address']+5+struct.unpack_from('<i',r['data'],1)[0]] for r in original if r['mnemonic']=='CALL'}
    assert list(source_calls.values())==list(original_calls.values())==['sound','notify','pool']
    proofs=[]
    for bits in range(64):
        inputs=[(bits>>i)&1 for i in range(6)]
        old=release_trace(original,original_calls,False,*inputs);new=release_trace(source,source_calls,True,*inputs)
        assert old==new,('Caller store/call/reload order differs',inputs,old,new)
        proofs.append(dict(sound_rprev_rnext_gprev_gnext_moved=inputs,events=new))
    exe_path=ROOT/'build/game/BHunter-source.exe';exe=exe_path.read_bytes();map_text=(ROOT/'build/game/BHunter-source.map').read_text()
    def address(name):
        found=re.findall(r'^\s+\w+:\w+\s+'+re.escape(name)+r'\s+([0-9a-fA-F]{8})\s',map_text,re.M);assert len(set(found))==1,name
        return int(found[0],16)
    verified=set();bindings=[]
    def linked_function(symbol,va):
        key=(symbol['name'],va)
        if key in verified:return
        verified.add(key);definition=sections[symbol['section']];code=bytearray(image_bytes(exe,va,len(definition['data'])))
        for r in definition['relocations']:
            at,s=r['offset'],r['symbol'];assert r['kind']==20
            assert struct.unpack_from('<I',definition['data'],at)[0]==0
            target=va+at+4+struct.unpack_from('<i',code,at)[0]
            if s['storage']==3:linked_function(functions[s['name']],target)
            else:assert target==address(s['name'])
            bindings.append(dict(owner=symbol['name'],offset=at,symbol=s['name'],linked_va=hex(target)))
            code[at:at+4]=definition['data'][at:at+4]
        assert code==definition['data'],('Linked function differs',symbol['name'])
    linked_function(owner,address(owner['name']));linked_function(functions['_bh_actor_vehicle_release'],address('_bh_actor_vehicle_release'))
    # These complete source-adapter bodies seal argument construction and the
    # post-query lookup. Lookup invariant: ECX=base+12*i, EAX=i, EDX=count;
    # compare record.world to the publisher's freshly read +1a4, then return
    # base+12*i on match or NULL. No call occurs inside the bounded search.
    notice=sections[functions['_vehicle_release_notice_room']['section']]
    lookup=sections[functions['_vehicle_release_room']['section']]
    assert notice['data'][:45].hex()=='8b44240c568b742408506a006a008b0e6a006a0051e8000000008b54242483c4185256e80000000083c4085ec3'
    assert [r['symbol']['name'] for r in notice['relocations']]==['_bh_entities_query','_vehicle_release_room']
    assert lookup['data'][:63].hex()=='8b4c24048b442408568b49048bb0a401000033c0578b510485d2761e8b398bcf3931740d4083c10c3bc272f433c05f5ec38d14408d04975f5ec35f5e33c0c3'
    assert not lookup['relocations']
    result=dict(original_va='004397e0',original_body_bytes=212,source_body_bytes=end,byte_match=False,
                classification='func/close with explicit source queue/query/room/pool ABI and unmapped-room guard',
                linked_matches_compared_object=True,actual_wrapper_calls_core=True,all_64_notification_list_paths=proofs,
                linked_callees_and_adapters=bindings,notice_adapter_query_arguments=['pool',0,0,0,0,'packet'],
                room_lookup_invariant='Fresh publisher +1a4 read after actual query; first matching record in stable unique-world registry, 12-byte stride.',
                differences='Source context arguments and room component lookup, smaller dead stack frame, register allocation and explicit post-query unmapped-room failure guard. Valid caller event/store/reload traces agree.',
                limits=['Not binary drop-in ABI or identical branch graph; source-only unmapped-room guard is explicit.',
                        'Sound, query dispatch and pool primitives are real shared source dependencies, not independently original-executed here.',
                        'Registered unique-world bindings and owner lifetimes must remain valid through callbacks; callback-visible room/link/count values may change.'],
                object_sha256=digest(obj),executable_sha256=digest(exe))
    (out/'linked-close.json').write_text(json.dumps(result,indent=2)+'\n');(out/'source.asm').write_text(listing)
    print(json.dumps({k:v for k,v in result.items() if k not in ('all_64_notification_list_paths','linked_callees_and_adapters')}))


if __name__=='__main__':main()
