"""Audit real native model composition, independent pixels and link evidence."""
import hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];FOLDER=Path(__file__).resolve().parent;OUT=ROOT/'build/model-draw-native'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
build=json.loads((FOLDER/'focused-build.json').read_text());assert build['passed'] and build['scenario']==dict(scenario='model-draw-native',checks=71,failures=0,original_code_required=False)
for source,digest in build['sources'].items():assert sha(ROOT/source)==digest,source
for name,record in build['objects'].items():assert sha(OUT/name)==record['sha256'],name
assert len([c for c in build['commands'] if c['label'].startswith('missing-') and c['exit_code'] and 'LNK2001' in c['output']])==3
link_map=(FOLDER/'link-map.txt').read_text()
for symbol in ('_bh_model_draw_lit','_bh_model_draw_unlit','_bh_model_draw_face','_bh_model_draw_leaf_chain','_bh_native_primitive_emit','_bh_light_evaluate_prepared','_bh_clip_polygon','_bh_texture_draw_polygon','_bh_texture_draw_flush','_bh_render_material_initialize','_bh_render_mode'):
    assert symbol in link_map,symbol
expected={'model-native-unlit-blue.bmp':0xff,'model-native-lit-red.bmp':0xff0000,'model-native-lit-green.bmp':0xff00,'model-native-point-blue.bmp':0xff,'model-native-mask-zero.bmp':0}
observations={}
for name,color in expected.items():
    path=FOLDER/name;assert sha(path)==build['bitmaps'][name];data=path.read_bytes();assert data[:2]==b'BM'
    offset=struct.unpack_from('<I',data,10)[0];width,height=struct.unpack_from('<ii',data,18);assert (width,height)==(64,64) and struct.unpack_from('<H',data,28)[0]==24
    stride=(width*3+3)&~3
    def pixel(x,y):
        at=offset+(height-1-y)*stride+x*3;b,g,r=data[at:at+3];return (r<<16)|(g<<8)|b
    assert pixel(32,28)==pixel(32,32)==color and pixel(4,4)==pixel(55,55)==0
    nonblack=sum(pixel(x,y)!=0 for y in range(height) for x in range(width));colors={pixel(x,y) for y in range(height) for x in range(width)}
    assert colors==({0,color} if color else {0})
    assert (300<nonblack<500) if color else not nonblack
    observations[name]=dict(center_rgb=f'{color:06x}',nonblack_pixels=nonblack,all_unique_rgb=[f'{c:06x}' for c in sorted(colors)],sha256=sha(path))
assert observations['model-native-lit-red.bmp']['center_rgb']!=observations['model-native-lit-green.bmp']['center_rgb']
assert build['bitmaps']['model-native-unlit-blue.bmp']==build['bitmaps']['model-native-point-blue.bmp']
for entry in json.loads((ROOT/'META/reference-manifest.json').read_text())['files']:
    if entry['root']=='repo':assert sha(ROOT/entry['path'])==entry['sha256']
result=dict(passed=True,target=build['target'],checks=71,failures=0,real_native_binding_validated=True,original_code_executed=False,game_launches=0,source_sha256=build['sources'],linked_source_object_snapshots=len(build['objects']),newly_compiled_real_tus=3,negative_link_controls=3,pixels=observations,limits=['One explicit typed BSP triangle fixture, real original camera/clipper/default material/state/native queue/RGB backend; this is not source-gameplay acceptance','Complete0046e390 software/native dispatch remains unfinished; selected original native branch is an explicitly documented adapter','Shared m0source objects are hash-snapshotted before linking; orphan bh_waypoint.obj lacks current source and is excluded explicitly','No fakeCOM/vtables or backend-success stubs; native API HRESULTs and actual readback pixels determine results','Independent real-archive world scene composition is separately parent-owned evidence, not inferred from this fixture'])
(FOLDER/'validation.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(passed=True,checks=71,negative_link_controls=3,real_native_binding_validated=True,bitmap_pixels={name:value['nonblack_pixels'] for name,value in observations.items()},original_code_executed=False,immutable_references_verified=True)))
