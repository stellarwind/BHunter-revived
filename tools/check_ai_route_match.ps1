$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$out = Join-Path $root 'build/ai-route-match'
New-Item -ItemType Directory -Force $out | Out-Null
$reference = Join-Path $root 'GAME_DATA/BHUNTER.EXE'
if ((Get-FileHash $reference).Hash -ne 'C86205E299119A66E27F687679EE7E978F26FA9A6C0BFB21F664AA6E74E848B1') { throw 'Reference differs' }
$bytes = [IO.File]::ReadAllBytes($reference)
$axis = [byte[]]$bytes[(0x4343c0-0x400c00)..(0x4343c0-0x400c00+297)]
$length = [byte[]]$bytes[(0x434810-0x400c00)..(0x434810-0x400c00+261)]
foreach ($offset in @(65,93,111,158,215,259)) {
    if ([BitConverter]::ToInt32($axis,$offset) -ne 0x489200) { throw 'Axis constant relocation differs' }
}
foreach ($offset in @(66,134)) {
    if ($length[$offset-1] -ne 0xe8 -or 0x434810+$offset+4+[BitConverter]::ToInt32($length,$offset) -ne 0x4341a0) { throw 'Curve callee differs' }
}
if ([BitConverter]::ToInt32($length,202) -ne 0x489218 -or [BitConverter]::ToInt32($length,225) -ne 0x48930c) { throw 'Length constants differ' }
# Original .rdata starts at VA489000, file offset87a00.
$zero = [byte[]]$bytes[(0x87a00+0x200)..(0x87a00+0x203)]
$quarter = [byte[]]$bytes[(0x87a00+0x218)..(0x87a00+0x21f)]
$one = [byte[]]$bytes[(0x87a00+0x30c)..(0x87a00+0x30f)]
if ([BitConverter]::ToSingle($zero,0) -ne 0 -or [BitConverter]::ToDouble($quarter,0) -ne -.25 -or [BitConverter]::ToSingle($one,0) -ne 1) { throw 'Original constant bytes differ' }
$header = 'static void install_original_route(unsigned char *p){unsigned long address;long delta;unsigned char axis[]={'+($axis -join ',')+'};unsigned char length[]={'+($length -join ',')+'};unsigned char zero[]={'+($zero -join ',')+'};unsigned char quarter[]={'+($quarter -join ',')+'};unsigned char one[]={'+($one -join ',')+'};memcpy(p,axis,sizeof(axis));memcpy(p+512,length,sizeof(length));memcpy(p+1024,zero,4);memcpy(p+1032,quarter,8);memcpy(p+1040,one,4);'
foreach ($offset in @(65,93,111,158,215,259)) { $header += 'address=(unsigned long)(p+1024);memcpy(p+'+$offset+',&address,4);' }
foreach ($offset in @(66,134)) { $header += 'delta=(long)bh_camera_curve-(long)(p+512+'+($offset+4)+');memcpy(p+512+'+$offset+',&delta,4);' }
$header += 'address=(unsigned long)(p+1032);memcpy(p+714,&address,4);address=(unsigned long)(p+1040);memcpy(p+737,&address,4);}'
[IO.File]::WriteAllText("$out/original-route.h",$header)
$vc = 'D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC'
$sp = 'D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH = "$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH"
$env:INCLUDE = "$sp/include;$vc/include"
$env:LIB = "$sp/lib;$vc/lib"
foreach ($name in @('CL','_CL_','LINK','_LINK_')) { [Environment]::SetEnvironmentVariable($name,$null,'Process') }
$names = @('bh_axis_snap','bh_route_curve_length','bh_camera_curve','bh_round','bh_fp')
$objects = @()
foreach ($name in ($names + @('ai_route_match'))) {
    $folder = if ($name -eq 'ai_route_match') { 'tests' } else { 'src' }
    & "$root/analysis/toolchain-sp3/bin/CL.EXE" /nologo /c /ML /O2 /G5 /Oi /Gy "/Fo$out/$name.obj" "$root/$folder/$name.c"
    if ($LASTEXITCODE) { throw "Compile failed: $name" }
    $objects += '"'+$out+'/'+$name+'.obj"'
}
$rsp = @('/nologo','kernel32.lib') + $objects + @('"/out:'+$out+'/ai-route-match.exe"','"/map:'+$out+'/ai-route-match.map"')
[IO.File]::WriteAllLines("$out/link.rsp",$rsp)
& "$vc/bin/LINK.EXE" "@$out/link.rsp"
if ($LASTEXITCODE) { throw 'Link failed' }
& "$out/ai-route-match.exe" | Tee-Object -FilePath "$out/result.log"
exit $LASTEXITCODE
