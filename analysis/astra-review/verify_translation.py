"""Build the saved source twice in fresh directories and require a full raw match."""
from pathlib import Path
import hashlib,json,os,subprocess,uuid
from targeted_probes import parse_coff

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
SOURCE=HERE/'matched_translation.cpp'
TOOLS=ROOT/'analysis/toolchain-sp3/bin'
INCLUDE=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc/include')
EXPECTED_EXE_SHA256='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
def sha(b):return hashlib.sha256(b).hexdigest()

def main():
    original=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert sha(original)==EXPECTED_EXE_SHA256
    target=original[0x860:0x860+45]
    run=HERE/('verified-'+uuid.uuid4().hex[:12]);run.mkdir()
    env=os.environ.copy();env.pop('CL',None);env.pop('_CL_',None)
    env['PATH']=str(TOOLS)+os.pathsep+env['PATH']
    env['INCLUDE']=str(INCLUDE)
    flags=['/nologo','/c','/ML','/Gy','/TP','/O2','/G5','/I'+str(INCLUDE)]
    manifest=dict(original_sha256=sha(original),original_va='00401460',original_file_offset='00000860',
        source=str(SOURCE),source_sha256=sha(SOURCE.read_bytes()),header=str(INCLUDE/'math.h'),
        header_sha256=sha((INCLUDE/'math.h').read_bytes()),target_hex=target.hex(),
        function_sha256=sha(target),comparison_mask_hex=(b'\xff'*45).hex(),relocations=[],
        tools={str(TOOLS/f):sha((TOOLS/f).read_bytes()) for f in ['CL.EXE','c1.dll','c1xx.dll','c2.exe']},builds=[])
    for i in range(2):
        folder=run/f'clean-{i+1}';folder.mkdir();obj=folder/'matched_translation.obj'
        command=[str(TOOLS/'CL.EXE'),*flags,'/Fo'+str(obj),str(SOURCE)]
        p=subprocess.run(command,cwd=folder,env=env,capture_output=True,text=True)
        (folder/'compiler.log').write_text(p.stdout+p.stderr)
        assert p.returncode==0,p.stdout+p.stderr
        symbols,sections=parse_coff(obj.read_bytes())
        symbol=next(s for s in symbols.values() if s['name']=='_bh_mat34_translate_world')
        section=sections[symbol['section']];code=section['data'][symbol['value']:]
        assert section['relocations']==[],section['relocations']
        assert code[:45]==target,(code.hex(),target.hex())
        assert all(x in (0x90,0xcc,0) for x in code[45:]),code.hex()
        row=dict(command=command,cwd=str(folder),returncode=p.returncode,object=str(obj),object_sha256=sha(obj.read_bytes()),
                 object_section_bytes=len(code),function_bytes=45,trailing_alignment_hex=code[45:].hex(),raw_match=True)
        manifest['builds'].append(row)
        print('VERIFIED',folder.name,'45/45 raw bytes; zero relocations; trailing alignment:',code[45:].hex())
    assert sha((ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes())==EXPECTED_EXE_SHA256
    (run/'verification.json').write_text(json.dumps(manifest,indent=2))
    (HERE/'latest-verification.json').write_text(json.dumps(manifest,indent=2))
    print('Evidence:',run/'verification.json')

if __name__=='__main__':main()
