"""Static whole linked damage-query section/dispatch audit. No game execution.

Instruction graph matching is deliberately separate and remains unproved here.
Switch data and compiler-local labels are retained within the owning function.
"""
import argparse
import json
from pathlib import Path
import re
import struct
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from compare_ai_pose import instructions
from compare_ai_guided import digest
from compare_ai_center import image_bytes
sys.path.insert(0,str(ROOT/'analysis/astra-review'))
from targeted_probes import parse_coff


def owner_instructions(path,symbols,sections,name):
    owner=next(s for s in symbols.values() if s['name']==name);section=sections[owner['section']]
    # Forward absolute references inside the owning COMDAT are its jump/index
    # tables. Selected LLVM symbols must also include every local code label.
    tables=[r['symbol']['value'] for r in section['relocations']
            if r['kind']==6 and r['symbol']['section']==owner['section'] and r['symbol']['value']>r['offset']]
    limit=min(tables) if tables else len(section['data'])
    selected=[name]+[s['name'] for s in symbols.values() if s['section']==owner['section'] and s['storage']==6 and s['value']<limit]
    rows,listing=instructions(path,','.join(selected));end=max(r['address']+len(r['data']) for r in rows if r['mnemonic']=='RET')
    rows=[r for r in rows if r['address']<end]
    assert b''.join(r['data'] for r in rows)==section['data'][:end]
    padding=section['data'][end:limit]
    assert set(padding)<={0x90} or padding in (bytes.fromhex('8bff'),bytes.fromhex('8d4900'))
    return rows,listing,end,limit


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--special',action='store_true',help='Inspect the complete special response query instead')
    parser.add_argument('--output-directory',type=Path)
    args=parser.parse_args();special=args.special
    out=args.output_directory or ROOT/('build/ai-query-match/response-link-audit' if special else 'build/ai-damage-query-match/link-audit');out.mkdir(parents=True,exist_ok=True)
    stem='bh_actor_response' if special else 'bh_actor_damage'
    original_va,original_length=(0x43a1d0,1793) if special else (0x435e80,1312)
    original_table,original_index,original_default=(0x43a8d4,0x43a904,0x43a8c9) if special else (0x4363a0,0x4363cc,0x436398)
    table_count=12 if special else 11
    path=ROOT/('build/game/'+stem+'.obj');obj=path.read_bytes();symbols,sections=parse_coff(obj)
    functions={s['name']:s for s in symbols.values() if s['section']>0 and s['type']==32}
    owner=functions['_'+stem+'_query_core'];section=sections[owner['section']]
    rows,listing,end,limit=owner_instructions(path,symbols,sections,owner['name'])
    exe=(ROOT/'build/game/BHunter-source.exe').read_bytes();map_text=(ROOT/'build/game/BHunter-source.map').read_text()
    reference=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert digest(reference)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    original=json.loads((ROOT/('META/m4/target656/response/original/0043a1d0.disasm.json' if special else 'META/m4/target600/b3/original/00435e80.disasm.json')).read_text())
    assert b''.join(bytes.fromhex(r['bytes']) for r in original)==image_bytes(reference,original_va,original_length)
    verified=set();bindings=[]
    def address(name):
        found=re.findall(r'^\s+\w+:\w+\s+'+re.escape(name)+r'\s+([0-9a-fA-F]{8})\s',map_text,re.M);assert len(found)==1,name
        return int(found[0],16)
    def linked(symbol,va):
        if (symbol['name'],va) in verified:return
        verified.add((symbol['name'],va));definition=sections[symbol['section']]
        code=bytearray(image_bytes(exe,va,len(definition['data'])))
        for r in definition['relocations']:
            offset,s=r['offset'],r['symbol'];addend=struct.unpack_from('<I',definition['data'],offset)[0]
            if r['kind']==20:
                assert code[offset-1]==0xe8
                target=va+offset+4+struct.unpack_from('<i',code,offset)[0]
                if s['storage']==3:linked(functions[s['name']],target)
                else:assert target==address(s['name'])
                bindings.append(dict(owner=symbol['name'],offset=offset,kind='real callee',symbol=s['name'],linked_va=hex(target)))
            else:
                assert r['kind']==6;target=struct.unpack_from('<I',code,offset)[0]
                if s['section']==symbol['section']:
                    assert target==va+s['value']+addend
                    bindings.append(dict(owner=symbol['name'],offset=offset,kind='own switch/branch target',source_relative_target=s['value']+addend))
                elif s['name'] in functions:
                    assert not addend
                    if s['storage']==3:linked(functions[s['name']],target)
                    else:assert target==address(s['name'])
                    bindings.append(dict(owner=symbol['name'],offset=offset,kind='real callback',symbol=s['name'],linked_va=hex(target)))
                else:
                    assert s['section']>0 and definition['data'][offset-2] in (0xd8,0xd9,0xdc,0xdd)
                    width=8 if definition['data'][offset-2] in (0xdc,0xdd) else 4
                    at=s['value']+addend;expected=sections[s['section']]['data'][at:at+width]
                    assert image_bytes(exe,target,width)==expected
                    bindings.append(dict(owner=symbol['name'],offset=offset,kind='initialized constant',constant_hex=expected.hex()))
            code[offset:offset+4]=definition['data'][offset:offset+4]
        assert code==definition['data'],('Linked function or attached switch data differs',symbol['name'])
    linked(owner,address(owner['name']));wrapper=functions['_'+stem+'_query']
    assert [r['symbol']['name'] for r in sections[wrapper['section']]['relocations']].count(owner['name'])==1
    linked(wrapper,address(wrapper['name']))
    # Actual compiler switch bindings, including all ignored and unsigned
    # out-of-range messages. Never mistake attached tables for code bytes.
    table_refs=sorted((r for r in section['relocations'] if r['kind']==6 and r['symbol']['section']==owner['section'] and r['symbol']['value']>r['offset']),key=lambda r:r['offset'])
    assert len(table_refs)==2
    index_reloc,table_reloc=table_refs
    index_at=index_reloc['symbol']['value'];table_at=table_reloc['symbol']['value'];assert table_at==limit and index_at>table_at and (index_at-table_at)%4==0
    source_table_count=(index_at-table_at)//4
    indices=section['data'][index_at:index_at+21];assert len(indices)==21 and max(indices)<source_table_count
    entries=[]
    for i in range(source_table_count):
        r=next(r for r in section['relocations'] if r['offset']==table_at+4*i)
        assert r['kind']==6 and r['symbol']['section']==owner['section']
        target=r['symbol']['value']+struct.unpack_from('<I',section['data'],r['offset'])[0]
        assert 0<=target<end;entries.append(target)
    old_indices=image_bytes(reference,original_index,21);old_table=image_bytes(reference,original_table,4*table_count)
    default=entries[indices[5]]
    active=(0,1,2,3,4,11,13,15,16,17,20) if special else (0,1,2,3,4,11,13,16,17,20)
    ignored=[i for i in range(21) if i not in active]
    assert all(entries[indices[i]]==default for i in ignored)
    assert all(struct.unpack_from('<I',old_table,old_indices[i]*4)[0]==original_default for i in ignored)
    tail=[r for r in rows if r['address']>=default]
    assert [(r['mnemonic'],r['operands'].upper()) for r in tail[:-2]]==[('POP','EDI'),('POP','ESI'),('POP','EBP'),('POP','EBX')]
    assert tail[-1]['mnemonic']=='RET' and tail[-2]['mnemonic']=='ADD' and tail[-2]['operands'].upper()==rows[0]['operands'].upper()
    assert rows[0]['mnemonic']=='SUB' and rows[0]['operands'].upper().startswith('ESP,')
    assert image_bytes(reference,original_default,8).hex()=='5f5e5d5b83c430c3'
    assert any(r['mnemonic']=='JA' and int(r['operands'],16)==default for r in rows[:12])
    result=dict(original_va=('%08x'%original_va),original_body_bytes=original_length,source_instruction_body_bytes=end,source_owned_section_bytes=len(section['data']),
                switch_table_bytes=4*source_table_count,original_switch_table_bytes=4*table_count,index_table_bytes=21,byte_match=False,whole_owner_cosmetic_only_proved=False,
                complete_linked_core_wrapper_helpers_callbacks_constants_and_tables_verified=True,actual_wrapper_calls_core=True,
                ignored_events=ignored,unsigned_out_of_range_returns_without_stores_or_calls=True,
                dispatch_targets={str(i):entries[indices[i]] for i in range(21)},object_sha256=digest(obj),executable_sha256=digest(exe),bindings=bindings,
                limits=['This proves actual linked definitions and no-op dispatch only; full x87/store/control graphs remain unproved.',
                        'Lifecycle, math and native evidence must retain separate scopes and dependencies.'])
    (out/'linked-core.json').write_text(json.dumps(result,indent=2)+'\n');(out/'source.asm').write_text(listing)
    print(json.dumps({k:v for k,v in result.items() if k not in ('bindings','dispatch_targets')}))


if __name__=='__main__':main()
