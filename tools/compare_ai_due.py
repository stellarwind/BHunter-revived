"""Static full due-owner clock/word-arithmetic/guard/linked-code proof; no execution."""
import json
import argparse
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


def timing_trace(rows,source,negative,success=True,inline=False):
    actor=('pointer','actor',0);context=('pointer','context',0);clock=('pointer','clock',0)
    regs={'ESP':0,'EBX':'saved.ebx','ESI':'saved.esi','EDI':'saved.edi'}
    if inline:regs['ESI']=actor
    memory={4:actor,8:context,12:clock,('clock',0):('clock32','entry'),
            ('actor',0x1e6):('word',('input16','duration')),('actor',0x1e4):('word',('input16','stamp'))}
    aliases={'AX':'EAX','BX':'EBX','CX':'ECX','DX':'EDX','SI':'ESI','DI':'EDI'}
    events=[];widths=[];flags=None;cursor=0;lookup={r['address']:i for i,r in enumerate(rows)};covered=set();delta_register='DI' if inline else 'SI'
    def low(value):
        if isinstance(value,int):return value&0xffff
        if isinstance(value,tuple) and value[0]=='word':return value[1]
        if isinstance(value,tuple) and value[0]=='clock32':return ('clock16',value[1])
        if isinstance(value,tuple) and value[0].endswith('16'):return value
        return ('low16',value)
    def operation(op,a,b):
        a,b=low(a),low(b)
        if op=='add16' and repr(a)>repr(b):a,b=b,a
        return (op,a,b)
    def address(operand):
        inner=re.search(r'\[(.*?)\]',operand)[1].strip()
        if inner=='0X004DFAC4':return ('clock',0)
        found=re.fullmatch(r'([A-Z]{3})(?:\s*\+\s*(0X[0-9A-F]+))?',inner);assert found,operand
        base=regs[found[1]];offset=int(found[2] or '0',16)
        return (base[1],base[2]+offset) if isinstance(base,tuple) and base[0]=='pointer' else base+offset
    def value(operand):
        if '[' in operand:
            at=address(operand);item=memory[at]
            if at==('clock',0):events.append(('read_clock_low16',low(item)));widths.append(16 if 'WORD PTR' in operand and 'DWORD' not in operand else 32)
            return low(item) if 'WORD PTR' in operand and 'DWORD' not in operand else item
        if operand in aliases:return low(regs[aliases[operand]])
        if operand in regs:return regs[operand]
        return int(operand,16)
    def write(operand,item):
        if '[' in operand:
            at=address(operand);assert at==('actor',0x1e4)
            memory[at]=('word',low(item));events.append(('stamp',low(item)))
        elif operand in aliases:regs[aliases[operand]]=('word',low(item))
        else:regs[operand]=item
    while cursor<len(rows):
        row=rows[cursor];cursor+=1;covered.add(row['address']);op,arg=row['mnemonic'],row['operands'].upper();parts=[p.strip() for p in arg.split(',')]
        if op=='BOUNDARY':
            assert inline and row==rows[-1] and regs['ESP']==0 and regs['ESI']==actor
            return dict(events=events,clock_read_widths=widths,covered=sorted(covered-{row['address']}))
        if op=='MOV':write(parts[0],value(parts[1]))
        elif op=='PUSH':
            item=value(arg);regs['ESP']-=4;memory[regs['ESP']]=item
        elif op=='POP':regs[arg]=memory[regs['ESP']];regs['ESP']+=4
        elif op=='SUB':
            assert parts[0]==delta_register;write(delta_register,operation('sub16',value(delta_register),value(parts[1])))
        elif op=='ADD':
            if parts[0]=='ESP':regs['ESP']+=int(parts[1],16)
            else:write(parts[0],('word',operation('add16',value(parts[0]),value(parts[1])))) if parts[0] not in aliases else write(parts[0],operation('add16',value(parts[0]),value(parts[1])))
        elif op=='TEST':
            assert parts[0]==parts[1]
            if parts[0]==delta_register:flags=('negative16',value(delta_register))
            else:assert parts[0]=='EAX';flags=('zero',value('EAX')==0)
        elif op=='JL':
            assert flags==('negative16',('sub16',('sub16',('clock16','entry'),('input16','duration')),('input16','stamp')))
            if negative:cursor=lookup[int(arg,16)]
        elif op in ('JNZ','JNE'):
            assert source and flags and flags[0]=='zero'
            if not flags[1]:cursor=lookup[int(arg,16)]
        elif op=='CALL':
            argc=2 if source else 1;args=[memory[regs['ESP']+4*i] for i in range(argc)]
            assert args==([context,actor] if source else [actor])
            events.append(('advance',actor));memory[('clock',0)]=('clock32','post')
            memory[('actor',0x1e4)]=('word',('input16','advance.stamp'))
            memory[('actor',0x1e6)]=('word',('input16','advance.duration'))
            regs.update(EAX=int(success),ECX='clobbered.ecx',EDX='clobbered.edx')
        elif op=='FSTP':
            # This interleaved pop is already interpreted by the motion-prefix
            # proof. It neither reads nor changes any integer clock operand.
            assert inline and arg=='ST0'
        elif op=='RET':
            assert regs['ESP']==0 and regs['ESI']=='saved.esi' and regs['EDI']=='saved.edi'
            if source:assert regs['EBX']=='saved.ebx' and regs['EAX']==(0 if not negative and not success else 1)
            return dict(events=events,clock_read_widths=widths)
        else:raise AssertionError(('Unreviewed timing instruction',row))
    raise AssertionError('No return')


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--object',type=Path,default=ROOT/'build/game/bh_actor_route_lifetime.obj');p.add_argument('--output-directory',type=Path,default=ROOT/'build/ai-due-match');p.add_argument('--ordinary-tick',action='store_true');a=p.parse_args()
    out=a.output_directory;out.mkdir(parents=True,exist_ok=True)
    path=a.object;obj=path.read_bytes();symbols,sections=parse_coff(obj)
    owner=next(s for s in symbols.values() if s['name']=='_bh_actor_route_due_core');section=sections[owner['section']]
    source,listing=instructions(path,owner['name']);end=max(r['address']+len(r['data']) for r in source if r['mnemonic']=='RET');source=[r for r in source if r['address']<end]
    assert b''.join(r['data'] for r in source)==section['data'][:end] and set(section['data'][end:])<={0x90}
    reference=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert digest(reference)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    original=[]
    for row in json.loads((ROOT/'META/m4/playable/ai-byte-match/00435790-asm.json').read_text()):
        va=int(row['address'],16)
        if va>=0x4357ca:continue
        data=bytes.fromhex(row['bytes']);assert data==image_bytes(reference,va,len(data));arg=', '.join(row['operands'])
        if row['mnemonic'].startswith('J'):arg=hex(int(arg,16)-0x435790)
        original.append(dict(address=va-0x435790,data=data,mnemonic=row['mnemonic'],operands=arg))
    assert b''.join(r['data'] for r in original)==image_bytes(reference,0x435790,58)
    original_calls=[0x435790+r['address']+5+struct.unpack_from('<i',r['data'],1)[0] for r in original if r['mnemonic']=='CALL']
    assert original_calls==[0x4355c0]
    proofs=[]
    for negative in (0,1):
        old=timing_trace(original,False,negative);new=timing_trace(source,True,negative)
        assert old['events']==new['events'],('Original valid path differs',old,new)
        proofs.append(dict(negative_delta=negative,original=old,source=new))
    failure=timing_trace(source,True,False,False)
    assert [event[0] for event in failure['events']]==['read_clock_low16','advance']
    inline_proofs=[]
    if a.ordinary_tick:
        start,stop=0x435940,0x435973;inline_rows=[]
        for row in json.loads((ROOT/'META/m4/target600/b3/original/004357d0.disasm.json').read_text()):
            va=int(row['address'],16)
            if not start<=va<stop:continue
            data=bytes.fromhex(row['bytes']);assert data==image_bytes(reference,va,len(data));arg=', '.join(row['operands'])
            if row['mnemonic'].startswith('J'):arg=hex(int(arg,16)-start)
            inline_rows.append(dict(address=va-start,data=data,mnemonic=row['mnemonic'],operands=arg))
        assert b''.join(r['data'] for r in inline_rows)==image_bytes(reference,start,stop-start)
        calls=[start+r['address']+5+struct.unpack_from('<i',r['data'],1)[0] for r in inline_rows if r['mnemonic']=='CALL'];assert calls==[0x4355c0]
        inline_rows.append(dict(address=stop-start,data=b'',mnemonic='BOUNDARY',operands=''));coverage=set()
        for negative in (0,1):
            old=timing_trace(inline_rows,False,negative,inline=True);new=timing_trace(source,True,negative)
            assert old['events']==new['events'];coverage.update(old['covered']);inline_proofs.append(dict(negative_delta=negative,original=old,source=new))
        assert coverage=={r['address'] for r in inline_rows[:-1]}
    exe_path=ROOT/'build/game/BHunter-source.exe';exe=exe_path.read_bytes();map_text=(ROOT/'build/game/BHunter-source.map').read_text()
    def address(name):
        found=re.findall(r'^\s+\w+:\w+\s+'+re.escape(name)+r'\s+([0-9a-fA-F]{8})\s',map_text,re.M);assert len(set(found))==1,name
        return int(found[0],16)
    va=address(owner['name']);linked=bytearray(image_bytes(exe,va,end));assert len(section['relocations'])==1
    r=section['relocations'][0];at,s=r['offset'],r['symbol'];assert r['kind']==20 and s['name']=='_bh_actor_route_advance'
    assert va+at+4+struct.unpack_from('<i',linked,at)[0]==address(s['name'])
    linked[at:at+4]=section['data'][at:at+4];assert linked==section['data'][:end]
    wrapper=next(s for s in symbols.values() if s['name']=='_bh_actor_route_due');ws=sections[wrapper['section']];wrapper_va=address(wrapper['name']);code=bytearray(image_bytes(exe,wrapper_va,len(ws['data'])));calls=[]
    for r in ws['relocations']:
        at,s=r['offset'],r['symbol'];assert r['kind']==20
        target=wrapper_va+at+4+struct.unpack_from('<i',code,at)[0]
        if s['storage']==3:
            definition=sections[s['section']];assert not definition['relocations'];body=definition['data'][s['value']:];assert image_bytes(exe,target,len(body))==body
        else:assert target==address(s['name'])
        calls.append(s['name']);code[at:at+4]=ws['data'][at:at+4]
    assert code==ws['data'] and owner['name'] in calls
    result=dict(original_va='00435790',original_body_bytes=58,source_body_bytes=end,byte_match=False,
                classification='func/close with explicit source clock/context ABI and retained checked-advance guard',
                linked_matches_compared_object=True,actual_wrapper_calls_core=True,valid_original_timing_paths=proofs,source_failure_guard=failure,
                differences='Explicit clock/context arguments, extra source advance-status guard and boolean return, saved clock pointer, final low-word load replacing an unobserved high-word read.',
                limits=['Not a binary drop-in ABI or identical branch graph: source error propagation is explicitly retained.',
                        'Original success/no-advance integer traces agree modulo 65536 for arbitrary entry/post clock values.',
                        'Stable real application clock binding required; no invented global, repeated catch-up loop or failed-advance stamp.'],object_sha256=digest(obj),executable_sha256=digest(exe))
    if a.ordinary_tick:
        result.update(ordinary_tick_inline_start='00435940',ordinary_tick_inline_end_exclusive='00435973',ordinary_tick_inline_bytes=51,ordinary_tick_all_integer_clock_instructions_covered=True,ordinary_tick_inline_timing_paths=inline_proofs,interleaved_fstp_owned_by_motion_prefix=True)
    (out/'linked-close.json').write_text(json.dumps(result,indent=2)+'\n');(out/'source.asm').write_text(listing)
    print(json.dumps({k:v for k,v in result.items() if k not in ('valid_original_timing_paths','source_failure_guard')}))


if __name__=='__main__':main()
