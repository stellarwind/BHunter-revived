"""Inside Blender, independently check saved mesh/UV/placement/packed assets."""
import bpy
import json
from pathlib import Path
import sys
from mathutils import Vector

out = Path(sys.argv[sys.argv.index('--')+1])
data = json.loads((out/'level-data.json').read_text())
for collection in bpy.data.collections:
    collection.hide_viewport = False
bpy.context.view_layer.update()
source = {model['name'].lower():model for model in data['models']}
objects = [obj for obj in bpy.data.objects if 'BHunter_placement_index' in obj]
assert {obj['BHunter_placement_index'] for obj in objects} == set(range(5815))
checked_vertices = checked_uvs = 0
for obj in objects:
    placement = data['placements'][obj['BHunter_placement_index']]
    model = source[placement['name'].lower()]
    x,y,z = model['vertices'][0]
    quarter = placement['flags']&3
    x,z = [(x,z),(z,-x),(-x,-z),(-z,x)][quarter]
    px,py,pz = placement['position']
    expected = Vector((x+px,z+pz,-y-py))
    actual = obj.matrix_world @ obj.data.vertices[0].co
    assert (actual-expected).length < .01, (obj.name,list(actual),list(expected))
    assert obj['BHunter_flags'] == placement['flags']
    checked_vertices += 1
for mesh in bpy.data.meshes:
    if 'BHunter_BSP' not in mesh: continue
    model = source[mesh['BHunter_BSP'].lower()]
    lookup = {face_id:i for i,face_id in enumerate(model['face_ids'])}
    for polygon in mesh.polygons:
        original_id = mesh.attributes['BHunter_face_index'].data[polygon.index].value
        original = lookup[original_id]
        material = mesh.materials[polygon.material_index]
        assert list(material['BHunter_material_bytes']) == model['materials'][model['face_materials'][original]]['attributes']
        assert len(polygon.loop_indices) == len(model['uvs'][original])
        for loop, uv in zip(polygon.loop_indices, model['uvs'][original]):
            actual = mesh.uv_layers[0].data[loop].uv
            assert abs(actual[0]-uv[0]) < .00001 and abs(actual[1]-uv[1]) < .00001
            checked_uvs += 1
packed = [image for image in bpy.data.images if image.packed_file]
assert len(packed) == len(data['textures']) == 351
assert all(image.size[0] > 0 and image.size[1] > 0 for image in packed)
report = dict(passed=True, saved_file=bpy.data.filepath, unique_placements=5815,
              placement_transform_checks=checked_vertices, uv_corner_checks=checked_uvs,
              packed_textures=len(packed), original_code_executed=False)
(out/'reopen-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
