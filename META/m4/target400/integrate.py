from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
modules=[('callback_helpers','callback-helpers','bh_callback_helpers_checkpoint'),('serial_options','serial-options','bh_serial_options_tests'),('script_lifetime','script-lifetime','bh_script_lifetime_checkpoint'),('menu_settings','menu-settings','bh_menu_settings_checkpoint')]
build=ROOT/'tools/m0.py';s=build.read_text()
for module,scenario,function in modules:
 if "('src/bh_"+module+".c'" not in s:
  s=s.replace('        for source, obj in modules:',"        modules += [('src/bh_"+module+".c','bh_"+module+".obj'),('tests/"+module+".c','"+module+"_tests.obj')]\n        for source, obj in modules:")
  s=s.replace("for scenario in (","for scenario in ('"+scenario+"',",1)
  s=s.replace("for name,symbol in [","for name,symbol in [('"+module+"','_"+function+"'),",1)
for before,after in [('bh_script_lifetime_checkpoint','bh_script_unloaded_query'),('bh_serial_options_tests','bh_serial_baud_label'),('bh_callback_helpers_checkpoint','bh_actor_target_removed'),('bh_menu_settings_checkpoint','bh_menu_network_open')]:
 s=s.replace("'_"+before+"'","'_"+after+"'")
build.write_bytes(s.encode('utf-8'))
p=ROOT/'tests/checkpoint.c';s=p.read_text()
for module,scenario,function in modules:
 if 'int '+function+'(' in s:continue
 s='int '+function+'(unsigned long *count);\n'+s
 s=s.replace('int main(int argc, char **argv) {','int main(int argc, char **argv) {\n    unsigned long '+module+'_checks=0;int '+module+'_failed;')
 s=s.replace('    pickup_basic_failed=','    '+module+'_failed='+function+'(&'+module+'_checks);\n    pickup_basic_failed=',1)
 # Add to the final summary counts, errors and process exit, not declarations.
 s=s.replace('rail_wagon_checks+script_wait_checks+',module+'_checks+rail_wagon_checks+script_wait_checks+',1)
 s=s.replace('rail_wagon_failed+script_wait_failed+',module+'_failed+rail_wagon_failed+script_wait_failed+',1)
 s=s.replace('return rail_wagon_failed ||','return '+module+'_failed || rail_wagon_failed ||',1) if 'return rail_wagon_failed ||' in s else s.replace('    return ','    return '+module+'_failed ||',1)
p.write_bytes(s.encode('utf-8'))
