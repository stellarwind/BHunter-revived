"""Stage one pinned official DDrawCompat release for M3 reference tests only."""
import hashlib
import io
import json
from pathlib import Path
import struct
import urllib.request
import zipfile

ROOT=Path(__file__).resolve().parents[1]
VERSION='v0.7.2'
NAME='DDrawCompat-v0.7.2.zip'
EXPECTED='a932a4dcb2100552b6918fd4a3cc7de138bb97448bbca2a7051954993a397b39'

def main():
    directory=ROOT/'build/m3/dependencies';directory.mkdir(exist_ok=True)
    url='https://api.github.com/repos/narzoul/DDrawCompat/releases/tags/'+VERSION
    request=urllib.request.Request(url,headers={'User-Agent':'BHunter-reference-research'})
    with urllib.request.urlopen(request,timeout=30) as response:release=json.load(response)
    asset=next(v for v in release['assets'] if v['name']==NAME)
    assert asset['digest']=='sha256:'+EXPECTED and release['tag_name']==VERSION
    archive=directory/NAME
    if archive.exists():data=archive.read_bytes()
    else:
        with urllib.request.urlopen(asset['browser_download_url'],timeout=30) as response:data=response.read()
        assert len(data)==asset['size'] and hashlib.sha256(data).hexdigest()==EXPECTED;archive.write_bytes(data)
    assert hashlib.sha256(data).hexdigest()==EXPECTED
    with zipfile.ZipFile(io.BytesIO(data)) as z:
        names=z.namelist();dll=next(n for n in names if Path(n).name.lower()=='ddraw.dll');payload=z.read(dll)
        pe=struct.unpack_from('<I',payload,0x3c)[0];assert payload[:2]==b'MZ' and payload[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',payload,pe+4)[0]==0x14c
        stage=directory/'DDrawCompat-v0.7.2';stage.mkdir(exist_ok=True)
        out=stage/'ddraw.dll'
        if out.exists():assert out.read_bytes()==payload
        else:out.write_bytes(payload)
        licenses=[n for n in names if 'license' in Path(n).name.lower()]
        for name in licenses:(stage/Path(name).name).write_bytes(z.read(name))
    report=dict(date='2026-10-24',scope='M3 isolated original-reference dependency only; not a source-game backend selection',version=VERSION,prerelease=release['prerelease'],published=release['published_at'],release_url=release['html_url'],source_repository='https://github.com/narzoul/DDrawCompat',asset_url=asset['browser_download_url'],asset_digest_from_github=asset['digest'],archive_sha256=hashlib.sha256(data).hexdigest(),dll_sha256=hashlib.sha256(payload).hexdigest(),dll_bytes=len(payload),machine='x86',staged_dll=out.relative_to(ROOT).as_posix(),license_files=licenses,zip_files=names,original_files_modified=False,normal_integration_dependency=False)
    evidence=ROOT/'META/m3/compat';evidence.mkdir(exist_ok=True);(evidence/'ddrawcompat-v0.7.2.json').write_text(json.dumps(report,indent=2)+'\n',newline='\n')
    print(json.dumps(report))

if __name__=='__main__':main()
