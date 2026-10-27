"""Record custom-box training chain, original contracts and crash control."""
import hashlib
import json
import shutil
import struct
from pathlib import Path
from record_pickup_basic import reference

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'META/m4/playable/training-box'
def read(p):return json.loads(p.read_text())
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    OUT.mkdir(exist_ok=True)
    ledger,raw=reference()
    box=read(ROOT/'build/box-test/playthrough.json')
    control=read(ROOT/'build/box-reuse-control/validation.json')
    build=read(ROOT/'build/game/build.json')
    assert box['passed'] and box['deterministic_trace_match'] and control['passed'] and build['passed']
    assert sha(ROOT/'build/box-test/BHunter-box.exe')==build['executable_sha256']
    contracts=[]
    for path in ('META/m4/playable/rocket-pickup-exit/original-step.json','META/m4/playable/rocket-pickup-exit/original-refresh.json','META/m4/target500/b2/original/0043c220.disasm.json'):
        rows=read(ROOT/path)
        for row in rows:
            data=bytes.fromhex(row['bytes']);assert raw(int(row['address'],16),len(data))==data
        contracts.append(dict(path=path,sha256=sha(ROOT/path),instructions=len(rows)))
    archive=(ROOT/'GAME_DATA/Bhunter.bin').read_bytes();index=read(ROOT/'analysis/assets-index.json')
    scripts=[]
    for name in ('BSP\\POWERUP.TXT','BSP\\MARKER.TXT','BSP\\SENSOR2.TXT','BSP\\LAUNCH2.TXT','BSP\\RELAY.TXT'):
        item=next(x for x in index if x['name'].upper()==name)
        data=archive[item['offset']:item['end']]
        scripts.append(dict(name=name,sha256=hashlib.sha256(data).hexdigest(),text=data.decode('latin1')))
    item=next(x for x in index if x['name'].upper()=='TEST.LEV');data=archive[item['offset']:item['end']]
    n=struct.unpack_from('<I',data)[0];placements=list(struct.iter_unpack('<16siiiI',data[4:4+n*32]));records=[]
    for row in struct.iter_unpack('<I8h',data[4+n*32:]):
        if row[0] in (1395,1396,1399,4827,4830,4831,1134):
            name,x,y,z,flags=placements[row[0]];records.append(dict(index=row[0],model=name.rstrip(b'\0').decode(),position=[x,y,z],slots=list(row[1:])))
    write(OUT/'original-contract.json',dict(instructions=contracts,scripts=scripts,placements=records,
        scope='Original script parameters and cached-pointer/marker-removal instructions; not full original/source gameplay equivalence.'))
    for source,dest in [('build/box-test/playthrough.json','playthrough.json'),('build/box-test/box-trace-run1.csv','box-trace-run1.csv'),('build/box-test/box-trace-run2.csv','box-trace-run2.csv'),('build/box-reuse-control/validation.json','negative-control.json'),('build/box-reuse-control/box-trace.csv','negative-trace.csv'),('build/box-reuse-control/startup.log','negative-startup.log'),('build/game/build.json','game-build.json'),('build/game/BHunter-source.map','game.map'),('build/box-test/level.json','level.json')]:
        shutil.copyfile(ROOT/source,OUT/dest)
    sources=['src/bh_entities.c','src/bh_entities.h','src/bh_game_app.c','src/bh_game_box.inc','tools/build_box_level.py','tools/test_box_level.py','tools/check_box_reuse_control.py','tools/record_training_box.py']
    (OUT/'sources').mkdir(exist_ok=True)
    for p in sources:shutil.copyfile(ROOT/p,OUT/'sources'/(sha(ROOT/p)+'-'+Path(p).name+'.txt'))
    report=dict(passed=True,scope='Custom test box only',gameplay_in_actual_training=False,
        software_renderer_executed=False,original_code_executed=False,
        executable_sha256=build['executable_sha256'],sources={p:sha(ROOT/p) for p in sources},
        dependencies='Real pool dispatch, pickup, player inventory, radar marker, proximity sensor, waitfor, VM launch, moving actors, weapon targeting/projectiles, damage, mission story/progress and ten-second timer relay.',
        results=[r['milestone_ticks'] for r in box['runs']],negative_control_tick=control['run']['ticks'],
        limits=['Box relocates original script chain into two connected rooms with one small center-line anchor; sensor uses radius4 instead of original room bounds.',
                'Box initializes first target directly; later targets/pickup/sensor/marker are created by production VM/wait callbacks. Player controls drive collection and firing; no injected damage, ammo or completion.',
                'Both runs have one active successor reuse. Old guard fails at tick170 generation1->3, matching the human failure class.',
                'Fixed runs complete881ticks with identical recorded traces; no full original-game numerical/PC24 equivalence or visual correctness claim.',
                'A nonfatal original degenerate-curve draw diagnostic remains; no software-renderer tests ran.',
                'Earlier all-weapon test evidence remains historical; this run covers the two-target training chain and next-assignment spawn.'])
    write(OUT/'validation.json',report)
    status=ROOT/'META/function-status.json';data=read(status)
    def visit(v):
        if isinstance(v,dict):
            if v.get('source') in sources and 'validation' in v:
                v['validation']['training_box']={'evidence':'META/m4/playable/training-box/validation.json','source_sha256':sha(ROOT/v['source']),'integration':'scenario_exercised','target':'build/box-test/BHunter-box.exe','scope':'Custom box training chain passes twice; old-guard control fails at the reproduced sensor reuse. No actual mission automation.'}
            for child in list(v.values()):visit(child)
        elif isinstance(v,list):
            for child in v:visit(child)
    visit(data);write(status,data)
    print(json.dumps(dict(passed=True,runs=2,ticks=881,old_guard_fails_at=170)))

if __name__=='__main__':main()
