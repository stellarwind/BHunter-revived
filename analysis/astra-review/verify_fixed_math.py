"""Verify the saved combined source twice, then audit isolated function context."""
from pathlib import Path
import hashlib,json,os,re,subprocess,uuid
from targeted_probes import parse_coff
from fixed_setup_math import ROOT,HERE,TOOLS,INCLUDE,FLAGS,EXPECTED_SHA,TARGETS

SOURCE=HERE/'matched_math.cpp'
FUNCTIONS={'bh_vec3_cross':'cross','bh_mat34_transform_point':'point','bh_mat34_transform_direction':'direction','bh_mat34_translate_world':'world'}
def sha(b):return hashlib.sha256(b).hexdigest()

def main():
    original=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert sha(original)==EXPECTED_SHA
    source=SOURCE.read_text()
    run=HERE/('verified-math-'+uuid.uuid4().hex[:12]);run.mkdir()
    env=os.environ.copy();env.pop('CL',None);env.pop('_CL_',None)
    env['PATH']=str(TOOLS)+os.pathsep+env['PATH'];env['INCLUDE']=str(INCLUDE)
    manifest=dict(original_exe_sha256=EXPECTED_SHA,source=str(SOURCE),source_sha256=sha(SOURCE.read_bytes()),
                  header=str(INCLUDE/'math.h'),header_sha256=sha((INCLUDE/'math.h').read_bytes()),
                  flags=FLAGS,tools={str(TOOLS/f):sha((TOOLS/f).read_bytes()) for f in ['CL.EXE','c1.dll','c1xx.dll','c2.exe']},builds=[])

    def build(label,src,names):
        folder=run/label;folder.mkdir();obj=folder/'math.obj'
        command=[str(TOOLS/'CL.EXE'),*FLAGS,'/Fo'+str(obj),str(src)]
        p=subprocess.run(command,cwd=folder,env=env,capture_output=True,text=True)
        (folder/'compiler.log').write_text(p.stdout+p.stderr)
        assert p.returncode==0,p.stdout+p.stderr
        symbols,sections=parse_coff(obj.read_bytes());functions=[]
        for name in names:
            s=next(s for s in symbols.values() if s['name']=='_'+name)
            section=sections[s['section']];code=section['data'][s['value']:]
            va,length=TARGETS[FUNCTIONS[name]];target=original[va-0x400c00:va-0x400c00+length]
            exact=not section['relocations'] and code[:length]==target and all(b in (0,0x90,0xcc) for b in code[length:])
            functions.append(dict(name=name,original_va=hex(va),target_bytes=length,object_section_bytes=len(code),
                                  original_hex=target.hex(),generated_hex=code.hex(),comparison_mask_hex=(b'\xff'*length).hex(),
                                  relocations=section['relocations'],raw_match=exact,function_sha256=sha(target),
                                  trailing_alignment_hex=code[length:].hex() if exact else None))
        result=dict(label=label,command=command,cwd=str(folder),source=str(src),source_sha256=sha(src.read_bytes()),
                    object=str(obj),object_sha256=sha(obj.read_bytes()),functions=functions)
        manifest['builds'].append(result)
        return functions

    for i in [1,2]:
        rows=build('combined-clean-'+str(i),SOURCE,list(FUNCTIONS))
        assert all(r['raw_match'] for r in rows),rows
        print('VERIFIED combined-clean-'+str(i)+': 4 functions, 335/335 raw instruction bytes, zero relocations.')

    # Preserve the same header/type preamble while isolating function bodies.
    preamble=source.split('// BEGIN ',1)[0]
    for name in FUNCTIONS:
        body=source.split('// BEGIN '+name+'\n',1)[1].split('// END '+name,1)[0]
        src=run/(name+'-isolated.cpp');src.write_text(preamble+body+'\n}\n')
        rows=build('isolated-'+FUNCTIONS[name],src,[name])
        if name!='bh_mat34_transform_direction':assert rows[0]['raw_match'],rows
        print('ISOLATED',name,'raw_match='+str(rows[0]['raw_match']))

    assert sha((ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes())==EXPECTED_SHA
    (run/'verification.json').write_text(json.dumps(manifest,indent=2))
    (HERE/'latest-math-verification.json').write_text(json.dumps(manifest,indent=2))
    print('Evidence:',run/'verification.json')

if __name__=='__main__':main()
