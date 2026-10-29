$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out=Join-Path $root 'build/ai-controls'
New-Item -ItemType Directory -Force -Path $out|Out-Null
$exeBytes=[IO.File]::ReadAllBytes("$root/GAME_DATA/BHUNTER.EXE")
if((Get-FileHash "$root/GAME_DATA/BHUNTER.EXE").Hash -ne 'C86205E299119A66E27F687679EE7E978F26FA9A6C0BFB21F664AA6E74E848B1'){throw 'Reference hash differs'}
$pe=[BitConverter]::ToInt32($exeBytes,0x3c)
$sectionAt=$pe+24+[BitConverter]::ToUInt16($exeBytes,$pe+20)
$sections=@(for($i=0;$i-lt[BitConverter]::ToUInt16($exeBytes,$pe+6);++$i){$at=$sectionAt+40*$i;@{rva=[BitConverter]::ToUInt32($exeBytes,$at+12);size=[BitConverter]::ToUInt32($exeBytes,$at+16);disk=[BitConverter]::ToUInt32($exeBytes,$at+20)}})
function Raw([int]$a,[int]$n){foreach($section in $sections){$offset=$a-0x400000-$section.rva;if($offset-ge0-and$offset+$n-le$section.size){$o=$section.disk+$offset;return ,([byte[]]$exeBytes[$o..($o+$n-1)])}};throw 'Reference file range missing'}
$header=[Collections.Generic.List[string]]::new();$fixes=[Collections.Generic.List[string]]::new();$constants=@{}
$header.Add('static void install_original(unsigned char *p){unsigned long address;long displacement;')
function Emit([byte[]]$bytes,[int]$at){$header.Add('{unsigned char bytes[]={'+($bytes -join ',')+'};memcpy(p+'+$at+',bytes,sizeof(bytes));}')}
# Reference wrapper preserves callee-saved registers and supplies original scratch slots.
$prefix=[Convert]::FromHexString('5355565781ec600100008bb424740100008bbc24780100008b07894424548b4704894424588b47088944245c')
Emit $prefix 0
$forcedPrefix=(Raw 0x438ff0 21)+[byte[]]@(0x57)
Emit $forcedPrefix 1024
$predictionPrefix=[Convert]::FromHexString('81ec4c010000535556578bb424600100008b842464010000')
Emit $predictionPrefix 28672
$firePrefix=[Convert]::FromHexString('81ec4c010000535556578bb424600100008b84246401000089442428')
Emit $firePrefix 30000
$ranges=@(
 @(0x437f17,0x438058,(30000+$firePrefix.Length),'META/m4/playable/ai-aggression/00437ae0-asm.json'),
 @(0x43950c,0x4397cf,(28672+$predictionPrefix.Length),'META/m4/playable/ai-aggression/00438ff0-asm.json'),
 @(0x437bf0,0x437cc5,$prefix.Length,'META/m4/playable/ai-aggression/00437ae0-asm.json'),
 @(0x439073,0x439184,(1024+$forcedPrefix.Length),'META/m4/playable/ai-aggression/00438ff0-asm.json'),
 @(0x4340f0,0x434199,2048,'META/m4/playable/ai-controls/clamp-asm.json'),
 @(0x4341a0,0x4343bf,4096,'META/m4/playable/ai-motion/curve-original-asm.json'),
 @(0x434810,0x434916,6144,'META/m4/playable/ai-controls/length-asm.json'),
 @(0x437ae0,0x437f17,16384,'META/m4/playable/ai-aggression/00437ae0-asm.json'),
 @(0x438fd8,0x438fe3,(16384+0x438fd8-0x437ae0),'META/m4/playable/ai-aggression/00437ae0-asm.json'),
 @(0x4011f0,0x401261,24576,'META/m4/playable/ai-controls/normalize-asm.json'),
 @(0x47de20,0x47de47,7168,'META/m4/playable/ai-controls/ftol-asm.json'))
$calls=@{0x4340f0=2048;0x4341a0=4096;0x434810=6144;0x47de20=7168;0x4011f0=24576;0x4011a0=25088}
$crossControl=Get-Content "$root/META/m4/playable/ai-byte-match/positive-control.json" -Raw|ConvertFrom-Json
$cross=Raw 0x4011a0 67
if(!$crossControl.matched-or$crossControl.original_length-ne67-or[Convert]::ToHexString($cross).ToLower()-ne$crossControl.original_hex){throw 'Complete cross-product positive control differs'}
Emit $cross 25088
foreach($range in $ranges){
 $start,$end,$destination,$file=$range
 Emit (Raw $start ($end-$start)) $destination
 $cursor=$start
 foreach($row in (Get-Content "$root/$file" -Raw|ConvertFrom-Json)){
  $at=[Convert]::ToInt32($row.address,16);if($at -lt $start -or $at -ge $end){continue}
  $bytes=[Convert]::FromHexString($row.bytes)
  if($cursor-ne$at-or[Convert]::ToHexString((Raw $at $bytes.Length))-ne$row.bytes.ToUpper()){throw "Reference/gap $($row.address)"}
  $cursor+=$bytes.Length
  if($row.mnemonic-eq'CALL' -and $row.operands[0] -match '^0x'){
   $callee=[Convert]::ToInt32($row.operands[0].Substring(2),16)
   if(-not$calls.ContainsKey($callee)){throw 'Unexpected pure-closure call'}
   $pos=$destination+$at-$start
   $fixes.Add('displacement='+$calls[$callee]+'-'+($pos+5)+';memcpy(p+'+($pos+1)+',&displacement,4);')
  }
  if($row.mnemonic-eq'PUSH'-and$row.operands[0]-eq'0x48c070'){
   $up=Raw 0x48c070 12
   if([BitConverter]::ToSingle($up,0)-ne0-or[BitConverter]::ToSingle($up,4)-ne1-or[BitConverter]::ToSingle($up,8)-ne0){throw 'Original up vector differs'}
   Emit $up 25600
   $fixes.Add('address=(unsigned long)(p+25600);memcpy(p+'+($destination+$at-$start+1)+',&address,4);')
  }
  foreach($operand in $row.operands){if($operand-match'\[0x([0-9a-fA-F]+)\]'){
   $address=[Convert]::ToInt32($Matches[1],16)
   if($address-eq0x4dfac4){$slot=12288}elseif($address-eq0x4dfab8){$slot=12292}else{
    if($address-lt0x489000-or$address-ge0x48b4e8){throw 'Unexpected global'}
    if(-not$constants.ContainsKey($address)){$constants[$address]=8192+16*$constants.Count;Emit (Raw $address 8) $constants[$address]}
    $slot=$constants[$address]
   }
   $index=-1;for($j=0;$j-le$bytes.Length-4;$j++){if([BitConverter]::ToInt32($bytes,$j)-eq$address){$index=$j;break}}
   if($index-lt0){throw 'Missing relocation'}
   $fixes.Add('address=(unsigned long)(p+'+$slot+');memcpy(p+'+($destination+$at-$start+$index)+',&address,4);')
  }}
 }
 if($cursor-ne$end){throw "Incomplete block $start"}
}
Emit ([Convert]::FromHexString('81c4600100005f5e5d5bc3')) ($prefix.Length+0x437cc5-0x437bf0)
Emit ([byte[]]@(0xef,0xbe,0,0)) 12288
# Return the four original stack vectors: fire, normalized direction, side,
# and travel. This adapter only copies outputs and restores saved registers.
Emit ([Convert]::FromHexString('8bbc24680100008d74241cb903000000f3a58d742410b903000000f3a58d742458b903000000f3a58db424a0000000b903000000f3a55f5e5d5b81c44c010000c3')) (30000+$firePrefix.Length+0x438058-0x437f17)
$header.AddRange($fixes);$header.Add('}');[IO.File]::WriteAllLines("$out/original.h",$header)
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC';$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH";$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($name in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($name,$null,'Process')}
$names=@('bh_actor_attack','bh_actor_response','bh_actor_pursuit_route','bh_entities','bh_camera_curve','bh_actor_route','bh_route_curve_length','bh_target_steering')
$objects=@()
foreach($name in ($names+@('ai_controls_original'))){
 $folder=if($name-eq'ai_controls_original'){'tests'}else{'src'}
 & "$root/analysis/toolchain-sp3/bin/CL.EXE" /nologo /c /ML /O2 /G5 /Oi /Gy /ID:/Decomp/dx7sdk/include /DDIRECT3D_VERSION=0x0500 /DDIRECTDRAW_VERSION=0x0500 "/Fo$out/$name.obj" "$root/$folder/$name.c"
 if($LASTEXITCODE){throw 'Compile failed'};$objects+='"'+$out+'/'+$name+'.obj"'
}
$exclude='runtime_fixes.obj|"/out:|"/map:|'+(($names|ForEach-Object{$_+'\.obj'})-join'|')
$rsp=@(Get-Content "$root/build/runtime-fixes/link.rsp"|Where-Object{$_-notmatch$exclude})
$rsp+=$objects;$rsp+='"/out:'+$out+'/ai-controls.exe"'
[IO.File]::WriteAllLines("$out/link.rsp",$rsp)
& "$vc/bin/LINK.EXE" "@$out/link.rsp";if($LASTEXITCODE){throw 'Link failed'}
Push-Location $out
try{& './ai-controls.exe'|Tee-Object -FilePath result.log;$result=$LASTEXITCODE}finally{Pop-Location}
exit $result

