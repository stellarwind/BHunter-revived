"""Admit each reviewed family against a fresh full integration result."""
import copy,hashlib,json,shutil,struct
from pathlib import Path
from record_pickup_basic import ROOT,reference,read,write,sha,active
HERE=ROOT/'META/m4/remaining129'

def runtime_classification(raw):
    report=read(HERE/'runtime-review.json');catalog=read(ROOT/'META/m1/role-catalog.json');changed=[]
    assert report['passed'] and sha(Path(report['library']))==report['library_sha256']
    for row in catalog['records']:
        address=row['address']
        if row['role']!='unknown' or int(address,16)<0x47db50:continue
        matches=report['matches'].get(address,[]);coverage=report['section_coverage'].get(address,[])
        if address=='004882d0':
            # Verify the import name through the actual PE import lookup table.
            exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();pe=struct.unpack_from('<I',exe,60)[0];rva=struct.unpack_from('<I',exe,pe+24+104)[0];found=False
            while True:
                lookup,_,_,_,iat=struct.unpack('<IIIII',raw(0x400000+rva,20))
                if not iat:break
                for i in range(4096):
                    value=struct.unpack('<I',raw(0x400000+(lookup or iat)+i*4,4))[0]
                    if not value:break
                    if 0x400000+iat+i*4==0x4890f0:
                        assert not(value&0x80000000)
                        assert raw(0x400000+value+2,10)==b'RtlUnwind\0';found=True
                rva+=20
            assert found and raw(0x4882d0,6)==bytes.fromhex('ff25f0904800')
            role='import_thunk';owner='KERNEL32.RtlUnwind';proof='Actual FF25 IAT thunk; PE import lookup at004890f0 resolves RtlUnwind.'
        elif coverage:
            role='shared_tail';owner='VC5 SP3 LIBC';proof='Every observed range is contained in freshly reverified complete library sections '+','.join(coverage)+'. Internal library labels are not independent game functions.'
        elif address=='00487e30':
            callers=[r for s in report['verified_sections'] for r in s['relocations'] if r['original_target']==address]
            assert any(r['symbol']=='__matherr' for r in callers)
            assert any(m['symbol']=='__matherr' for m in matches) and raw(0x487e30,3)==bytes.fromhex('33c0c3')
            role='library_verified';owner='VC5 SP3 LIBC _matherr';proof='Three-byte body alone is nonunique; identity additionally established by actual __matherr relocation from the fully verified CRT math-error caller.'
        elif address=='0047e3ca':
            match=next(m for m in matches if m['symbol']=='__CIatan2');assert match['unmasked_bytes']==2
            assert raw(0x47e3ca,10)==bytes.fromhex('ba80304900e9ec450000')
            assert any(s['start']=='004829c0' for s in report['verified_sections'])
            role='library_verified';owner='VC5 SP3 LIBC intrinsic math wrapper';proof='Full two-instruction descriptor/tail-dispatch wrapper matches library wrappers and jumps to verified __cintrindisp2. Exact math alias remains unresolved; no unique atan2 claim from two unmasked opcode bytes.'
        else:
            full=[m for m in matches if m['unmasked_bytes']>=6];assert full,address
            role='library_verified';owner='VC5 SP3 LIBC';proof='All observed ranges match library code, masking only actual COFF relocation operands. Candidate symbols: '+', '.join(sorted({m['symbol'] for m in full}))+'. Symbol aliases are not uniquely attributed.'
        row.update(role=role,owner=owner,provenance_confidence='verified library/import boundary; see explicit proof and alias limits',boundary_review='reviewed',behavior=proof,assigned_next=False)
        row.setdefault('evidence',[]).append('META/m4/remaining129/runtime-review.json');changed.append(dict(address=address,role=role,proof=proof))
    assert len(changed)==50
    write(HERE/'runtime-classification.json',dict(original_function_admissions=0,entries=changed,library_entries=49,import_thunks=1))
    write(ROOT/'META/m1/role-catalog.json',catalog)

def main():
    ledger,raw=reference();build=read(ROOT/'build/m0/result.json');current=active(ledger)
    assert len(current)==600 and build['passed'] and not build['original_code_required']
    assert build['test']['failures']==0
    for row in current:assert row['source_sha256']==sha(ROOT/row['source'])==build['sources'][row['source']],row['id']
    scenario=next(s for s in build['scenarios'] if s.get('scenario')=='impact-ring');assert scenario['checks']==99 and not scenario['failures']
    source='src/bh_impact_ring.c';address='00407930';body=raw(int(address,16),227)
    instructions=read(HERE/'original'/f'{address}.disasm.json')
    assert b''.join(bytes.fromhex(r['bytes']) for r in instructions)==body and instructions[-1]['mnemonic']=='RET'
    assert raw(0x489420,8)==struct.pack('<d',.7) and raw(0x48c080,12)==struct.pack('<fff',0,0,1)
    assert '_bh_impact_ring_draw ' in (ROOT/'build/m0/bhunter-integration.map').read_text(errors='replace')
    assert build['missing_impact_ring_control']=='rejected with LNK2001'
    identifier=address+'-functional-c-remaining129'
    ledger['records'].append(dict(id=identifier,original_va=address,function='bh_impact_ring_draw',source=source,source_sha256=sha(ROOT/source),role='game',provenance_confidence='Complete effect draw callback installed at original player update sites0041f957/0041fb28; actual geometry/queue arithmetic, not a CRT/import/shared tail.',implementation='func',integration='linked_harness',target='build/m0/bhunter-integration.exe',game_admitted=False,validation=dict(boundary=dict(start=address,end_exclusive='00407a13',bytes=227,sha256=hashlib.sha256(body).hexdigest()),abi='cdecl; explicit borrowed queue/resource/yaw replaces globals; entity fields+100/+104/+108/+114 verified at compile time',fixtures=scenario,original_code_executed=False,raw_byte_match=False,limits=['Source-only harness, not playable game admission.','PC53 finite nonzero-duration contract; invalid duration/age/size/yaw rejected before enqueue.','Actual game render appearance remains unvalidated.']),evidence=['META/m4/remaining129/impact-ring-validation.json','META/m4/remaining129/original/00407930.disasm.json']))
    snapshots={}
    for name in ('src/bh_impact_ring.c','src/bh_impact_ring.h','tests/impact_ring.c','tests/checkpoint.c','tools/m0.py'):
        assert build['sources'].get(name,sha(ROOT/name))==sha(ROOT/name)
        dest=HERE/'sources'/(sha(ROOT/name)+'-'+Path(name).name+'.txt');dest.parent.mkdir(exist_ok=True);shutil.copyfile(ROOT/name,dest);snapshots[name]=dest.relative_to(ROOT).as_posix()
    write(HERE/'impact-ring-integration.json',build);shutil.copyfile(ROOT/'build/m0/bhunter-integration.map',HERE/'impact-ring-link-map.txt')
    write(HERE/'impact-ring-validation.json',dict(passed=True,baseline_functions=600,active_original_functions=601,whole_new_owners=1,real_total=build['test'],source_snapshots=snapshots,source_sha256=sha(ROOT/source),original_code_executed=False,game_launches=0))
    runtime_classification(raw)
    ledger['updated']='2026-10-26';ledger['current_source_frontier']='META/m4/remaining129/NEXT.md';write(ROOT/'META/function-status.json',ledger)
    print(json.dumps(dict(active_original_functions=601,new_source_owners=1,runtime_entries_excluded=50,remaining_catalog_entries=78)))
if __name__=='__main__':main()
