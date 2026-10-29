"""Static complete candidate instruction comparison under one reviewed register bijection.

This candidate is uninstalled; its proof is not production-query evidence.
"""
import json
import re
from pathlib import Path
from compare_ai_damage_query import owner_instructions, parse_coff, image_bytes, digest

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'META/m4/playable/collision-close/traversal'


def normalize(mnemonic,operand,rename=False):
    mnemonic={'JNZ':'JNE','JZ':'JE'}.get(mnemonic,mnemonic)
    operand=operand.upper().replace('FLOAT PTR','DWORD PTR').replace('DOUBLE PTR','QWORD PTR')
    if mnemonic=='MOV':operand=operand.replace('DWORD PTR','')
    operand=re.sub(r'\bST\(([0-7])\)',r'ST\1',operand)
    operand=re.sub(r'\bST\b','ST0',operand)
    if mnemonic=='FXCH' and not operand:operand='ST1'
    if mnemonic in ('FMUL','FADD','FSUB','FSUBR') and re.fullmatch(r'ST[0-7]',operand):operand='ST0, '+operand
    if mnemonic in ('FADDP','FMULP') and not operand:operand='ST1, ST0'
    if rename:
        operand=re.sub(r'\b(EBX|EDI)\b',lambda m:{'EBX':'EDI','EDI':'EBX'}[m[0]],operand)
    # Canonicalize only the spelling of integer address sums/scales.
    def address(m):
        terms=[]
        for term in m[1].replace(' ','').split('+'):
            factors=term.split('*')
            factors=[hex(int(f,16)) if f.startswith('0X') else hex(int(f)) if f.isdigit() else f for f in factors]
            terms.append('*'.join(sorted(factors)))
        return '['+'+'.join(sorted(terms))+']'
    operand=re.sub(r'\[([^]]+)\]',address,operand)
    operand=re.sub(r'0X[0-9A-F]+',lambda m:hex(int(m[0],16)),operand)
    operand=re.sub(r'\s+','',operand)
    return mnemonic,operand


def main():
    obj=ROOT/'build/collision-context-probe/query.obj'
    symbols,sections=parse_coff(obj.read_bytes());symbol=next(s for s in symbols.values() if s['name']=='_bh_collision_traversal_core')
    source,listing,end,_=owner_instructions(obj,symbols,sections,symbol['name']);section=sections[symbol['section']]
    assert end==391
    old=json.loads((ROOT/'META/m4/collision-traversal/original/0044d2f0.disassembly.json').read_text())
    image=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert digest(image)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    assert b''.join(bytes.fromhex(r['bytes']) for r in old)==image_bytes(image,0x44d2f0,391)
    globals_={'_bh_traversal_a':0x4e0fcc,'_bh_traversal_b':0x4e0fd0,'_bh_traversal_relative':0x4e1010}
    calls_={'_bh_mat34_transform_point':0x4012e0,'_bh_traversal_node':0x44c200,'_bh_collision_traversal_core':0x44d2f0}
    bindings={}
    for r in section['relocations']:
        row=next(s for s in source if s['address']<=r['offset']<s['address']+len(s['data']))
        name=r['symbol']['name'];at=r['offset'];assert section['data'][at:at+4]==b'\0'*4
        if r['kind']==20:
            assert row['mnemonic']=='CALL' and name in calls_;row['operands']=hex(calls_[name])
        else:
            assert r['kind']==6 and name in globals_
            assert '0x0' in row['operands'];row['operands']=row['operands'].replace('0x0',hex(globals_[name]))
        bindings[at]=dict(symbol=name,original_address=hex((calls_ if r['kind']==20 else globals_)[name]))
    proof=[]
    for original,current in zip(old,source):
        at=int(original['address'],16)-0x44d2f0
        assert at==current['address'] and len(bytes.fromhex(original['bytes']))==len(current['data'])
        operands=', '.join(original['operands'])
        if original['mnemonic'].startswith('J'):operands=hex(int(operands,16)-0x44d2f0)
        # ABI save/restore slots retain physical registers. All body uses,
        # including PUSH call arguments, follow the single EBX<->EDI mapping.
        abi=(at in (3,4,9) or current['mnemonic']=='POP')
        left=normalize(original['mnemonic'],operands)
        right=normalize(current['mnemonic'],current['operands'],rename=not abi)
        assert left==right,(hex(at),left,right)
        proof.append(dict(offset=at,instruction=left,abi_save_restore=abi))
    assert len(old)==len(source)==147
    OUT.mkdir(parents=True,exist_ok=True)
    result=dict(passed=True,static_only=True,candidate_installed=False,object_sha256=digest(obj.read_bytes()),
        original_bytes=391,source_bytes=end,instructions=147,register_bijection={'EBX':'EDI','EDI':'EBX'},
        bindings=bindings,proof=proof,byte_match=False,
        limits=['Proof belongs only to the uninstalled recursive candidate, not the production iterative execute function',
                'Original global field views and transform/destination callees remain explicit dependency contracts',
                'No floating arithmetic canonicalization: identical actual FP instructions after register renaming'])
    (OUT/'candidate-graph.json').write_text(json.dumps(result,indent=2)+'\n')
    (OUT/'candidate.asm.txt').write_text(listing)
    print('All147 complete candidate instructions agree under the reviewed body register bijection; candidate remains uninstalled')


if __name__=='__main__':main()
