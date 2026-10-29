param([switch]$Response)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent;$out=Join-Path $root $(if($Response){'build/ai-response-velocity-match'}else{'build/ai-damage-velocity-match'})
New-Item -ItemType Directory -Force $out|Out-Null
$reference=Join-Path $root 'GAME_DATA/BHUNTER.EXE'
if((Get-FileHash $reference).Hash-ne'C86205E299119A66E27F687679EE7E978F26FA9A6C0BFB21F664AA6E74E848B1'){throw 'Reference differs'}
$bytes=[IO.File]::ReadAllBytes($reference)
function Raw([int]$a,[int]$n){$offset=if($a-lt0x489000){$a-0x400c00}else{$a-0x489000+555520};return ,([byte[]]$bytes[$offset..($offset+$n-1)])}
# Native-only cdecl adapter copies six input words to the original frame slots.
# The complete93-byte arithmetic window then runs without fabricated callees.
$prefix=[Convert]::FromHexString('81eca00000008b8424a40000008b9424a80000008b08894c24288b4804894c242c8b4808894c24308b0a894c24648b4a04894c24688b4a08894c246c')
$suffix=[Convert]::FromHexString('8b8c24a40000008b44244489018b4424488941048b44244c89410881c498000000c3')
$start=0x435bf9;$end=0x435c56;$file='META/m4/target600/b3/original/004357d0.disasm.json'
if($Response){
 $start=0x439e7a;$end=0x439ee0;$file='META/m4/target656/response/original/00439910.disasm.json'
 $prefix=[Convert]::FromHexString('81ecb00000008b8424b40000008b9424b80000008b08894c24288b4804894c242c8b4808894c24308b0a898c24880000008b4a04898c248c0000008b4a08898c2490000000')
 $suffix=[Convert]::FromHexString('8b8c24b40000008b44244489018b4424488941048b44244c89410881c4a8000000c3')
}
$body=Raw $start ($end-$start);$constant=Raw 0x489c68 8
if([BitConverter]::ToDouble($constant,0)-ne15){throw 'Original scale differs'}
$cursor=$start;$relocs=[Collections.Generic.List[int]]::new()
foreach($row in(Get-Content "$root/$file" -Raw|ConvertFrom-Json)){
 $at=[Convert]::ToInt32($row.address,16);if($at-lt$start-or$at-ge$end){continue};$data=[Convert]::FromHexString($row.bytes)
 if($at-ne$cursor-or[Convert]::ToHexString((Raw $at $data.Length))-ne$row.bytes.ToUpper()){throw 'Original arithmetic gap/mismatch'};$cursor+=$data.Length
 if($row.mnemonic-eq'CALL'-or$row.mnemonic-match'^J'){throw 'Unexpected window dependency/control flow'}
 foreach($operand in $row.operands){if($operand-match'0x00489c68'){$relocs.Add($prefix.Length+$at-$start+2)}}
}
if($cursor-ne$end-or$relocs.Count-ne3){throw 'Incomplete arithmetic window'}
$header=@('static void install_original_velocity(unsigned char *p){unsigned long address=(unsigned long)(p+512);')
$header+=('{unsigned char bytes[]={'+(($prefix+$body+$suffix)-join',')+'};memcpy(p,bytes,sizeof(bytes));}')
$header+=('{unsigned char bytes[]={'+($constant-join',')+'};memcpy(p+512,bytes,sizeof(bytes));}')
foreach($at in $relocs){$header+=('memcpy(p+'+$at+',&address,4);')};$header+='}'
[IO.File]::WriteAllLines("$out/original-velocity.h",$header)
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC';$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH";$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($name in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($name,$null,'Process')}
$build=Get-Content "$root/build/game/build.json" -Raw|ConvertFrom-Json
if(!$build.passed-or$build.debugging-or(Get-FileHash "$root/src/bh_actor_damage.c").Hash.ToLower()-ne$build.sources.'src/bh_actor_damage.c'){throw 'Current release game objects required'}
if((Get-FileHash "$root/src/bh_actor_damage_tick_core.inc").Hash.ToLower()-ne$build.sources.'src/bh_actor_damage_tick_core.inc'){throw 'Rebuild changed damage tick core first'}
if($Response){foreach($source in @('src/bh_actor_response.c','src/bh_actor_response_finish.inc')){if((Get-FileHash "$root/$source").Hash.ToLower()-ne$build.sources.$source){throw 'Rebuild changed response owner first'}}}
$defines=@(if($Response){'/DRESPONSE_VELOCITY_ORIGINAL'})
& "$root/analysis/toolchain-sp3/bin/CL.EXE" /nologo /c /ML /O2 /G5 /Oi /Gy /ID:/Decomp/dx7sdk/include /DDIRECT3D_VERSION=0x0500 /DDIRECTDRAW_VERSION=0x0500 @defines "/Fo$out/ai_damage_velocity_original.obj" "$root/tests/ai_damage_velocity_original.c";if($LASTEXITCODE){throw 'Compile failed'}
$rsp=@($build.commands[-1].argv|Select-Object -Skip 1|Where-Object{$_-notmatch'bh_game_main.obj$|/out:|/map:|/subsystem:'}|ForEach-Object{'"'+$_+'"'})
$rsp+='/subsystem:console';$rsp+=('"'+$out+'/ai_damage_velocity_original.obj"');$rsp+=('"/out:'+$out+'/ai-damage-velocity-match.exe"');$rsp+=('"/map:'+$out+'/ai-damage-velocity-match.map"');[IO.File]::WriteAllLines("$out/link.rsp",$rsp)
& "$vc/bin/LINK.EXE" "@$out/link.rsp";if($LASTEXITCODE){throw 'Link failed'}
& "$out/ai-damage-velocity-match.exe"|Tee-Object "$out/result.log";exit $LASTEXITCODE
