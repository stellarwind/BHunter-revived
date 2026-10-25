"""Update current repository summaries from the admitted timed-block evidence."""
import json,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
FOLDER=Path(__file__).resolve().parent
v=json.loads((FOLDER/'validation.json').read_text())
n=v['complete_new_functions'];total=182+n;tu=v['translation_units'];real=v['real_total']['checks'];clean=v['clean_total']['checks'];controls=v['missing_module_controls']
metrics=f"{total}/727 admitted original functions in {tu} translation units; {real:,} archive checks, {clean:,} clean checks and {controls} missing-module link controls."
summary=f"The timed scene-caller dependency block restores {n} complete owners: {metrics} Every admitted variant compiles in its real translation unit, links against real definitions in build/m0/bhunter-integration.exe, and passes affected source-built checks. Real and clean source hashes agree. No original code or game execution occurred."
frontier="Both scene callbacks `0041c860`/`00428f50` remain unimplemented. Full horizon/software rasterization, HUD/handoff, player/flyby constructors, physical input collection, weapon lifetimes and startup resource/script bindings remain open. Next recover a complete dependency family on that frontier; weighted target helper `00421610` is reviewed but unimplemented. PC24, CD audio and DirectPlay requirements remain. M3 is skipped; M4 is incomplete. This timed block is stopped pending user resumption."
p=ROOT/'README.md';s=p.read_text();a=s.index('**Latest iteration');b=s.index('[Progress & blockers]',a)
s=s[:a]+f"**Latest iteration · 2026-10-25:** {n} complete scene-caller dependencies now include player/input initialization, all camera modes, world collision correction, flyby/route updates, real background resources, atlas text, horizon color tables and collision cache initialization. Both full scene callbacks remain open.\n\n**Validation:** {tu} translation units · {real:,} real-asset checks · {clean:,} clean checks · {controls} missing-module controls. No game launches.\n\n**Next:** horizon/software rendering and actual caller/constructor dependencies toward a playable slice.\n\n"+s[b:];p.write_text(s)
p=ROOT/'META/PROGRESS.md';s=p.read_text();s=re.sub(r'Updated 2026-10-24\.', 'Updated 2026-10-25.',s,count=1)
s=s.replace('## Current checkpoint\n','## Current checkpoint\n\nTimed source restoration, 2026-10-25: **'+summary+'**\n\n'+frontier+' Evidence: `m4/scene-callers/README.md`, `m4/scene-callers/validation.json`; audit `python tools/check_scene_callers.py`.\n',1);p.write_text(s)
p=ROOT/'RESTORATION_PLAN.md';s=p.read_text().replace('182/727 admitted original functions in a 163-TU source harness',f'{total}/727 admitted original functions in a {tu}-TU source harness',1)
heading='## 8. Next execution block — M4 decompilation and source restoration\n'
s=s.replace(heading,heading+'\nCurrent checkpoint, 2026-10-25: '+summary+' '+frontier+' Evidence: `META/m4/scene-callers/`.\n\nEarlier checkpoint history follows.\n',1);p.write_text(s)
p=ROOT/'BUILDING.md';s=p.read_text().replace('The current 132-TU source-only scenario passes','The historical 132-TU source-only scenario passes',1)
s+='\nScene-caller dependency audit (2026-10-25): `python tools/check_scene_callers.py` verifies '+metrics+' All new source families link into the real integration target. The asset-free Git-index checkout has identical source hashes; the archive-only route fixture explicitly reports its absent-archive skip, while synthetic route tests remain required. Native fixtures use actual assets and include blank controls. Input evaluation consumes explicit snapshots and does not collect devices. The lazy collision cache does not silently replace existing query metadata. Both full scene callers and horizon/software rendering remain unfinished. No original code or gameplay is executed. Evidence: `META/m4/scene-callers/`.\n';p.write_text(s)
p=FOLDER/'README.md';s=p.read_text();s=s.replace('Timed source-restoration block, 2026-10-25.',summary+'\n\nTimed source-restoration block, 2026-10-25.',1);p.write_text(s)
print(metrics)
