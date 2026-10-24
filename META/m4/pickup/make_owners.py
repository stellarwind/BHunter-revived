"""Rebuild the 18-owner pickup source manifest from immutable EXE listings."""
import csv
import hashlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent
ORIGINAL=HERE/'original'

# Address, source function, role, completed source behavior, scope note.
OWNERS=[
    ('00441a00','bh_pickup_spawn','game',True,'Pickup constructor and room/global registration.'),
    ('004415a0','bh_pickup_tick','game',True,'Pickup step, offer/query and lifetime behavior.'),
    ('00441570','bh_pickup_query','game',True,'Pickup query event 20 transition.'),
    ('004414d0','bh_pickup_cleanup','game',True,'Pickup unlink and release callback.'),
    ('004412f0','bh_pickup_draw','game',True,'Default model, glow and quantity-label submission.'),
    ('00440c40','bh_pickup_label','game',True,'Two projected numeric/unit atlas label layers.'),
    ('004558d0','bh_pickup_project','engine_candidate',True,'Projected-camera point helper; engine provenance unverified.'),
    ('00478f60','bh_pickup_rect_queue','engine_candidate',True,'Queued rectangle producer and bucket insertion; engine provenance unverified.'),
    ('004776c0','bh_pickup_rect_execute','engine_candidate',True,'Complete queue callback; downstream rectangle renderer has native-only backend limit.'),
    ('00454fc0','bh_pickup_rect_draw','engine_candidate',False,'Native geometry/clip/UV/shade adapter only; original software rasterization remains WIP.'),
    ('00409f20','bh_entity_schedule_tick','game',True,'Thirty-frame room callback countdown.'),
    ('00409f40','bh_entity_schedule','game',True,'Resource, actor, pickup, linked-room and portal scheduling.'),
    ('00409530','bh_entity_schedule_clip_portal','engine_candidate',True,'Portal-specific convex clipper composition; only mapped caller is the scheduler.'),
    ('00478990','bh_draw_controls_state','engine_candidate',True,'Texture-mask command producer.'),
    ('00478a70','bh_draw_controls_clip_rect','engine_candidate',True,'Viewport rectangle command producer.'),
    ('00458300','bh_draw_controls_texture_mask','engine_candidate',True,'Texture-mask propagation over explicit resource bindings.'),
    ('00477f40','bh_draw_controls_execute','engine_candidate',True,'Texture-mask queued callback branch.'),
    ('00477e40','bh_draw_controls_execute','engine_candidate',True,'Viewport rectangle callback branch and next-rectangle suppression.'),
]
SOURCES={
    'bh_pickup_spawn':'src/bh_pickup.c','bh_pickup_tick':'src/bh_pickup.c',
    'bh_pickup_query':'src/bh_pickup.c','bh_pickup_cleanup':'src/bh_pickup.c',
    'bh_pickup_draw':'src/bh_pickup_draw.c','bh_pickup_label':'src/bh_pickup_draw.c',
    'bh_pickup_project':'src/bh_pickup_draw.c','bh_pickup_rect_queue':'src/bh_pickup_draw.c',
    'bh_pickup_rect_execute':'src/bh_pickup_draw.c','bh_pickup_rect_draw':'src/bh_pickup_draw.c',
    'bh_entity_schedule_tick':'src/bh_entity_schedule.c',
    'bh_entity_schedule':'src/bh_entity_schedule.c',
    'bh_entity_schedule_clip_portal':'src/bh_entity_schedule.c',
    'bh_draw_controls_state':'src/bh_draw_controls.c',
    'bh_draw_controls_clip_rect':'src/bh_draw_controls.c',
    'bh_draw_controls_texture_mask':'src/bh_draw_controls.c',
    'bh_draw_controls_execute':'src/bh_draw_controls.c',
}

def sha(data):return hashlib.sha256(data).hexdigest()

entries=[]
for address,function,role,complete,note in OWNERS:
    tsv=ORIGINAL/(address+'.tsv');decomp=ORIGINAL/(address+'.c.txt')
    rows=list(csv.DictReader(tsv.open(encoding='utf-8'),delimiter='\t'))
    if not rows or not decomp.exists():raise RuntimeError(f'Missing original listing: {address}')
    raw=bytearray();cursor=int(address,16)
    for row in rows:
        if int(row['address'],16)!=cursor:raise RuntimeError(f'{address}: listing gap at {cursor:08x}')
        instruction=bytes.fromhex(row['bytes']);raw.extend(instruction);cursor+=len(instruction)
    source=SOURCES[function]
    entries.append({
        'address':address,'end_exclusive':f'{cursor:08x}',
        'bytes':len(raw),'instructions':len(rows),'body_sha256':sha(raw),
        'role':role,'role_confidence':'high',
        'source':source,'function':function,'complete':complete,'scope':note,
        'original_tsv':str(tsv.relative_to(ROOT)).replace('\\','/'),
        'original_decompilation':str(decomp.relative_to(ROOT)).replace('\\','/'),
        'source_sha256':sha((ROOT/source).read_bytes()),
    })
result={
    'reference':'GAME_DATA/BHUNTER.EXE',
    'reference_sha256':'c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1',
    'body_hash_method':'SHA-256 of instruction bytes concatenated in increasing VA; contiguous listing verified',
    'source_variant':'functional source, native-only rectangle adapter explicitly partial',
    'count':len(entries),'complete_count':sum(e['complete'] for e in entries),
    'owners':entries,
}
assert result['count']==18 and result['complete_count']==17
(HERE/'owners.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8',newline='\n')
print(f"{result['count']} owners, {result['complete_count']} complete; {sum(e['bytes'] for e in entries)} original bytes")
