"""Attach the four closed source families to the real integration target."""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
modules=[('editor_input','editor-input','bh_editor_input_checkpoint','_bh_editor_input_setup'),('script_attached_sound','script-attached-sound','bh_script_attached_sound_checkpoint','_bh_script_attached_sound_tick'),('script_motion','script-motion','bh_script_motion_checkpoint','_bh_script_translate_tick'),('display_enum','display-enum','bh_display_enum_checkpoint','_bh_display_catalog_callback@16')]
p=ROOT/'tools/m0.py';s=p.read_text()
for module,scenario,function,symbol in modules:
    if "('src/bh_"+module+".c'" in s:continue
    s=s.replace("        # Keep every actual translation unit", "        modules += [('src/bh_"+module+".c','bh_"+module+".obj'),('tests/"+module+".c','"+module+"_tests.obj')]\n        # Keep every actual translation unit",1)
    s=s.replace("for header in (","for header in ('bh_"+module+".h',",1)
    s=s.replace("for scenario in (","for scenario in ('"+scenario+"',",1)
    s=s.replace("for name,symbol in [","for name,symbol in [('"+module+"','"+symbol+"'),",1)
p.write_bytes(s.encode('utf-8'))
p=ROOT/'tests/checkpoint.c';s=p.read_text()
for module,scenario,function,symbol in modules:
    if 'int '+function+'(' in s:continue
    s='int '+function+'(unsigned long *count);\n'+s
    s=s.replace('int main(int argc, char **argv) {','int main(int argc, char **argv) {\n    unsigned long '+module+'_checks=0;int '+module+'_failed;',1)
    s=s.replace('    pickup_basic_failed=','    '+module+'_failed='+function+'(&'+module+'_checks);\n    pickup_basic_failed=',1)
    s=s.replace('rail_wagon_checks+script_wait_checks+',module+'_checks+rail_wagon_checks+script_wait_checks+',1)
    s=s.replace('rail_wagon_failed+script_wait_failed+',module+'_failed+rail_wagon_failed+script_wait_failed+',1)
    s=s.replace('    return ','    return '+module+'_failed ||',1)
p.write_bytes(s.encode('utf-8'))
