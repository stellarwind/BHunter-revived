"""Record one whole-owner family against the current complete source build."""
import argparse,hashlib,re,shutil
from pathlib import Path
from record_pickup_basic import ROOT,reference,read,write,sha,active
HERE=ROOT/'META/m4/remaining129'
def main():
    parser=argparse.ArgumentParser();parser.add_argument('spec');args=parser.parse_args()
    spec=read(HERE/args.spec);ledger,raw=reference();current=active(ledger);build=read(ROOT/'build/m0/result.json')
    assert len(current)==spec['baseline'] and build['passed'] and not build['original_code_required'] and build['test']['failures']==0
    for r in current:assert r['source_sha256']==sha(ROOT/r['source'])==build['sources'][r['source']],r['id']
    scenario=next(s for s in build['scenarios'] if s.get('scenario')==spec['scenario']);assert scenario['checks']>0 and scenario['failures']==0
    assert build['missing_'+spec['module']+'_control']=='rejected with LNK2001'
    linkmap=(ROOT/'build/m0/bhunter-integration.map').read_text(errors='replace')
    for owner in spec['owners']:
        address=owner['original_va'];assert address not in {r['original_va'] for r in current}
        rows=read(HERE/'original'/(address+'.disasm.json'));body=b''.join(bytes.fromhex(r['bytes']) for r in rows)
        assert len(body)==owner['bytes'] and raw(int(address,16),len(body))==body
        assert rows[-1]['mnemonic']=='RET';assert re.search(r'(?<!\w)_'+owner['function']+r'(?=\s)',linkmap)
        owner=dict(owner);owner.pop('bytes');owner.update(id=address+'-functional-c-remaining129',source_sha256=sha(ROOT/owner['source']),implementation='func',integration='linked_harness',target='build/m0/bhunter-integration.exe',game_admitted=False)
        owner['validation']=dict(boundary=dict(start=address,end_exclusive=f'{int(address,16)+len(body):08x}',bytes=len(body),sha256=hashlib.sha256(body).hexdigest()),fixtures=scenario,limits=spec['limits'],raw_byte_match=False,original_code_executed=False)
        owner['evidence']=['META/m4/remaining129/'+spec['module']+'-validation.json','META/m4/remaining129/original/'+address+'.disasm.json']
        ledger['records'].append(owner)
    for constant in spec.get('constants',[]):assert raw(int(constant['address'],16),len(bytes.fromhex(constant['hex'])))==bytes.fromhex(constant['hex'])
    snapshots={};sources={}
    for name in spec['sources']+['tests/checkpoint.c','tools/m0.py']:
        digest=sha(ROOT/name);assert build['sources'].get(name,digest)==digest
        dest=HERE/'sources'/(digest+'-'+Path(name).name+'.txt');shutil.copyfile(ROOT/name,dest)
        snapshots[name]=dest.relative_to(ROOT).as_posix();sources[name]=digest
    write(HERE/(spec['module']+'-integration.json'),build);shutil.copyfile(ROOT/'build/m0/bhunter-integration.map',HERE/(spec['module']+'-link-map.txt'))
    write(HERE/(spec['module']+'-validation.json'),dict(passed=True,baseline_functions=len(current),active_original_functions=len(active(ledger)),whole_new_owners=len(spec['owners']),source_sha256=sources,source_snapshots=snapshots,real_total=build['test'],scenario=scenario,limits=spec['limits'],original_code_executed=False,game_launches=0))
    ledger['updated']='2026-10-26';write(ROOT/'META/function-status.json',ledger)
    print('Admitted',len(active(ledger)),'whole source owners;',scenario['scenario'],scenario['checks'],'checks')
if __name__=='__main__':main()
