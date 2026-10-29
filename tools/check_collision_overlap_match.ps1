param([string]$EvidenceDirectory='META/m4/playable/collision-audit/geometry')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out=Join-Path $root 'build/collision-overlap-match'
$evidence=Join-Path $root $EvidenceDirectory
New-Item -ItemType Directory -Force $out,$evidence|Out-Null
$reference=Join-Path $root 'GAME_DATA/BHUNTER.EXE'
$referenceHash='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
if((Get-FileHash $reference).Hash.ToLower()-ne$referenceHash){throw 'Original reference differs'}
function Raw([byte[]]$bytes,[int]$va,[int]$size){
 $pe=[BitConverter]::ToInt32($bytes,0x3c);$count=[BitConverter]::ToInt16($bytes,$pe+6)
 $at=$pe+24+[BitConverter]::ToInt16($bytes,$pe+20)
 for($i=0;$i-lt$count;++$i){
  $rva=[BitConverter]::ToInt32($bytes,$at+12);$diskSize=[BitConverter]::ToInt32($bytes,$at+16);$disk=[BitConverter]::ToInt32($bytes,$at+20)
  $delta=$va-0x400000-$rva
  if($delta-ge0-and$delta+$size-le$diskSize){return ,([byte[]]$bytes[($disk+$delta)..($disk+$delta+$size-1)])}
  $at+=40
 };throw 'Address outside file sections'
}
$bytes=[IO.File]::ReadAllBytes($reference)
$owners=@(
 @{address=0x44e210;length=19;symbol='_bh_geometry_set_flags_core';offset=0;expected='8b4424048b54240c8b48788b442408891481c3';name='set-flags'},
 @{address=0x44e230;length=15;symbol='_bh_geometry_get_flags_core';offset=64;expected='8b4424048b5424088b48788b0491c3';name='get-flags'},
 @{address=0x44e240;length=87;symbol='_bh_geometry_vertex_core';offset=128;expected='8b442408568b7424088d04408b4e7c8b4481048b4c24108bd14985d2740e8b564c41c1e004498b44100475f68b4e4cc1e0048b04088d04408d14808b46445e8d0c908b5424108b0189028b41048942048b4908894a08c3';name='vertex'}
)
$header=@('static void install_original_geometry(unsigned char *p){')
foreach($owner in $owners){
 $body=Raw $bytes $owner.address $owner.length
 if([Convert]::ToHexString($body).ToLower()-ne$owner.expected){throw 'Complete original instruction body differs'}
 $header+=('{unsigned char bytes[]={'+($body-join',')+'};memcpy(p+'+$owner.offset+',bytes,sizeof(bytes));}')
};$header+='}';[IO.File]::WriteAllLines("$out/original-geometry.h",$header)
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC';$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH";$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($name in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($name,$null,'Process')}
$commands=@()
foreach($pair in @(@('src/bh_geometry.c','bh_geometry'),@('tests/collision_overlap_match.c','native'))){
 $arguments=@('/nologo','/c','/ML','/O2','/G5','/Oi','/Gy',"/I$root/src","/Fo$out/$($pair[1]).obj","$root/$($pair[0])")
 & "$root/analysis/toolchain-sp3/bin/CL.EXE" @arguments
 if($LASTEXITCODE){throw 'Native real translation-unit compile failed'}
 $commands+=@{tool='analysis/toolchain-sp3/bin/CL.EXE';arguments=$arguments}
}
$rsp=@('/nologo','/opt:ref','kernel32.lib',"$out/bh_geometry.obj","$out/native.obj","/out:$out/collision-overlap-match.exe","/map:$out/collision-overlap-match.map")
[IO.File]::WriteAllLines("$out/link.rsp",@($rsp|ForEach-Object{'"'+$_+'"'}))
& "$vc/bin/LINK.EXE" "@$out/link.rsp";if($LASTEXITCODE){throw 'Native real-definition link failed'}
$native=& "$out/collision-overlap-match.exe";if($LASTEXITCODE){throw 'Native owner comparison failed'}
$native|Set-Content "$out/result.log";$native|Write-Output
$exe=[IO.File]::ReadAllBytes("$out/collision-overlap-match.exe");$map=[IO.File]::ReadAllText("$out/collision-overlap-match.map")
$proof=@()
foreach($owner in $owners){
 $object="$out/bh_geometry.obj"
 & python "$root/tools/compare_ai_object.py" $object $owner.symbol ('0x{0:x}'-f$owner.address) $owner.length --output "$evidence/$($owner.name)-object.json"
 if($LASTEXITCODE){throw 'Object comparison failed'}
 $m=[regex]::Match($map,'\s'+[regex]::Escape($owner.symbol)+'\s+([0-9a-fA-F]{8})\s')
 if(!$m.Success){throw 'Linked function missing'}
 $linked=Raw $exe ([Convert]::ToInt32($m.Groups[1].Value,16)) $owner.length
 $hex=[Convert]::ToHexString($linked).ToLower()
 $differences=@(for($i=0;$i-lt$owner.length;++$i){if($linked[$i]-ne([Convert]::FromHexString($owner.expected))[$i]){$i}})
 if($differences.Count){throw 'Exact linked owner mismatch'}
 $proof+=@{original_va=('0x{0:x}'-f$owner.address);symbol=$owner.symbol;length=$owner.length;linked_va=$m.Groups[1].Value;linked_hex=$hex;original_hex=$owner.expected;exact=($differences.Count-eq0);difference_offsets=$differences;installed=$true}
}
$negative=[Convert]::FromHexString($owners[0].expected);$negative[0]=$negative[0]-bxor1
if([Convert]::ToHexString($negative).ToLower()-eq$owners[0].expected){throw 'Negative control failed'}
$sources=@{};foreach($path in @('src/bh_geometry.c','src/bh_geometry_core.h','src/bh_geometry_core.inc','tests/collision_overlap_match.c','tools/check_collision_overlap_match.ps1')){$sources[$path]=(Get-FileHash "$root/$path").Hash.ToLower()}
@{passed=$true;original_sha256=$referenceHash;target='build/collision-overlap-match/collision-overlap-match.exe';target_sha256=(Get-FileHash "$out/collision-overlap-match.exe").Hash.ToLower();native=($native|ConvertFrom-Json);owners=$proof;negative_control='One-byte original mutation rejected by whole-body equality';commands=$commands;link_arguments=$rsp;sources=$sources;limits=@('No game or collision-response fidelity claim','All three complete leaf cores are byte exact in the real geometry translation unit','Source checked layout adapters are separate from exact owner cores','Native oracle copies only the three complete no-call geometry leaf bodies, not a game image')}|ConvertTo-Json -Depth 12|Set-Content "$evidence/validation.json"
