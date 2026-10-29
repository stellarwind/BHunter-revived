# Isolated complete original selector; no game, renderer, input, audio or network.
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent;$out=Join-Path $root 'build/ai-selection-match'
New-Item -ItemType Directory -Force $out|Out-Null
if((Get-FileHash "$root/GAME_DATA/BHUNTER.EXE").Hash-ne'C86205E299119A66E27F687679EE7E978F26FA9A6C0BFB21F664AA6E74E848B1'){throw 'Reference differs'}
$image=[IO.File]::ReadAllBytes("$root/GAME_DATA/BHUNTER.EXE");$pe=[BitConverter]::ToInt32($image,0x3c);$sectionAt=$pe+24+[BitConverter]::ToUInt16($image,$pe+20)
$parts=@(for($i=0;$i-lt[BitConverter]::ToUInt16($image,$pe+6);++$i){$at=$sectionAt+40*$i;@{rva=[BitConverter]::ToUInt32($image,$at+12);size=[BitConverter]::ToUInt32($image,$at+16);disk=[BitConverter]::ToUInt32($image,$at+20)}})
function Raw([int]$a,[int]$n){foreach($s in $parts){$o=$a-0x400000-$s.rva;if($o-ge0-and$o+$n-le$s.size){return ,([byte[]]$image[($s.disk+$o)..($s.disk+$o+$n-1)])}};throw 'Original range absent'}
$rows=Get-Content "$root/META/m4/playable/ai-aggression/00421610-asm.json" -Raw|ConvertFrom-Json
$header=[Collections.Generic.List[string]]::new();$header.Add('static void install_selection_original(unsigned char *p){unsigned long address;unsigned char bytes[]={'+((Raw 0x421610 304)-join',')+'};memcpy(p,bytes,sizeof(bytes));')
$cursor=0x421610
foreach($row in $rows){$at=[Convert]::ToInt32($row.address,16);if($at-lt0x421610-or$at-ge0x421740){continue};$bytes=[Convert]::FromHexString($row.bytes);if($cursor-ne$at-or[Convert]::ToHexString((Raw $at $bytes.Length))-ne$row.bytes.ToUpper()){throw 'Original instruction gap/mismatch'};$cursor+=$bytes.Length
 foreach($arg in $row.operands){if($arg-match'\[0x([0-9a-fA-F]+)\]'){$address=[Convert]::ToInt32($Matches[1],16);$slot=switch($address){0x4dfab8{512}0x4dfb28{516}default{throw 'Unreviewed selector global'}};$offset=-1;for($j=0;$j-le$bytes.Length-4;++$j){if([BitConverter]::ToUInt32($bytes,$j)-eq$address){$offset=$j;break}};if($offset-lt0){throw 'Operand absent'};$header.Add('address=(unsigned long)(p+'+$slot+');memcpy(p+'+($at-0x421610+$offset)+',&address,4);')}}
}
if($cursor-ne0x421740){throw 'Incomplete original owner'};$header.Add('}');[IO.File]::WriteAllLines("$out/original.h",$header)
$build=Get-Content "$root/build/game/build.json" -Raw|ConvertFrom-Json
if(!$build.passed-or$build.debugging){throw 'Current release build required'}
foreach($p in @('src/bh_target_selection.c','src/bh_entities.c','src/bh_fp.c')){if((Get-FileHash "$root/$p").Hash.ToLower()-ne$build.sources.$p){throw 'Rebuild changed production source first'}}
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC';$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH";$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($n in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($n,$null,'Process')}
& "$root/analysis/toolchain-sp3/bin/CL.EXE" /nologo /c /ML /O2 /G5 /Oi /Gy /ID:/Decomp/dx7sdk/include /DDIRECT3D_VERSION=0x0500 /DDIRECTDRAW_VERSION=0x0500 "/Fo$out/ai_selection_original.obj" "$root/tests/ai_selection_original.c"
if($LASTEXITCODE){throw 'Compile failed'}
$rsp=@($build.commands[-1].argv|Select-Object -Skip 1|Where-Object{$_-notmatch'bh_game_main.obj$|/out:|/map:|/subsystem:'}|ForEach-Object{'"'+$_+'"'});$rsp+='/subsystem:console';$rsp+=('"'+$out+'/ai_selection_original.obj"');$rsp+=('"/out:'+$out+'/ai-selection-match.exe"');$rsp+=('"/map:'+$out+'/ai-selection-match.map"');[IO.File]::WriteAllLines("$out/link.rsp",$rsp)
& "$vc/bin/LINK.EXE" "@$out/link.rsp";if($LASTEXITCODE){throw 'Link failed'}
Push-Location $out
try{& "$out/ai-selection-match.exe"|Tee-Object "$out/native.log";$result=$LASTEXITCODE}finally{Pop-Location}
exit $result
