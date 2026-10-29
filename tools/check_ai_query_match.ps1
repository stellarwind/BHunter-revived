param([switch]$Lifecycle,[switch]$Damage,[switch]$DamageLifecycle)
$ErrorActionPreference='Stop'
if($DamageLifecycle){$Damage=$true}
if($Lifecycle-and$Damage){throw 'Select one owner family'}
$root=Split-Path $PSScriptRoot -Parent
$out=Join-Path $root $(if($Damage){'build/ai-damage-query-match'}else{'build/ai-query-match'})
New-Item -ItemType Directory -Force $out | Out-Null
$reference=Join-Path $root 'GAME_DATA/BHUNTER.EXE'
if((Get-FileHash $reference).Hash-ne'C86205E299119A66E27F687679EE7E978F26FA9A6C0BFB21F664AA6E74E848B1'){throw 'Reference differs'}
$image=[IO.File]::ReadAllBytes($reference)
$optional=[BitConverter]::ToInt32($image,0x3c)+24
$imageEnd=0x400000+[BitConverter]::ToUInt32($image,$optional+56)
function Raw([int]$address,[int]$count){$offset=if($address-lt0x489000){$address-0x400c00}else{$address-0x489000+555520};return ,([byte[]]$image[$offset..($offset+$count-1)])}
$header=[Collections.Generic.List[string]]::new()
$fixes=[Collections.Generic.List[string]]::new()
$constants=@{}
$header.Add('static void install_original_query(unsigned char *p){unsigned long address,old;long displacement;int i;')
function Emit([byte[]]$bytes,[int]$at){$header.Add('{unsigned char bytes[]={'+($bytes-join',')+'};memcpy(p+'+$at+',bytes,sizeof(bytes));}')}
$ranges=@(@(0x43a1d0,0x43a8d1,0,'META/m4/target656/response/original/0043a1d0.disasm.json'),@(0x421610,0x421740,2560,'META/m4/playable/ai-aggression/00421610-asm.json'),@(0x4341a0,0x4343bf,4096,'META/m4/playable/ai-motion/curve-original-asm.json'),@(0x47de20,0x47de47,6144,'META/m4/playable/ai-controls/ftol-asm.json'))
if($Damage){$ranges=@(@(0x435e80,0x4363a0,0,'META/m4/target600/b3/original/00435e80.disasm.json'),@(0x4341a0,0x4343bf,4096,'META/m4/playable/ai-motion/curve-original-asm.json'))}
if($Lifecycle){$ranges+=,@(0x4397e0,0x4398b4,14336,'META/m4/target600/b2/original/004397e0.disasm.json')}
if($DamageLifecycle){$rows=@(Import-Csv "$root/META/m4/target-steering/original/00435720.tsv" -Delimiter "`t"|ForEach-Object{$parts=$_.instruction-split' ',2;[pscustomobject]@{address=$_.address;bytes=$_.bytes;mnemonic=$parts[0];operands=@($parts[1]-split',')}});$releaseRows=$rows;$ranges+=,@(0x435720,0x435786,14336,$null)}
foreach($range in $ranges){
 $start,$end,$destination,$file=$range
 Emit (Raw $start ($end-$start)) $destination
 $cursor=$start
 $rows=if($file){Get-Content "$root/$file" -Raw|ConvertFrom-Json}else{$releaseRows}
 foreach($row in $rows){
  $at=[Convert]::ToInt32($row.address,16);if($at-lt$start-or$at-ge$end){continue}
  $bytes=[Convert]::FromHexString($row.bytes)
  if($cursor-ne$at-or[Convert]::ToHexString((Raw $at $bytes.Length))-ne$row.bytes.ToUpper()){throw 'Original instruction gap/mismatch'}
  $cursor+=$bytes.Length
  $pos=$destination+$at-$start
  if($row.mnemonic-eq'CALL'-and$row.operands[0]-match'^0x'){
   $callee=[Convert]::ToInt32($row.operands[0].Substring(2),16)
   $target=switch($callee){0x4341a0{'(long)(p+4096)'}0x47de20{'(long)(p+6144)'}0x421610{'(long)(p+2560)'}0x476e90{if($Lifecycle-or$DamageLifecycle){'(long)oracle_set_query'}else{'(long)oracle_unavailable'}}0x47c140{if($Lifecycle-or$DamageLifecycle){'(long)oracle_sound_stop'}else{'(long)oracle_unavailable'}}0x477070{if($Lifecycle-or$DamageLifecycle){'(long)oracle_release'}else{'(long)oracle_unavailable'}}0x4770f0{'(long)oracle_broadcast'}default{throw 'Unreviewed original call'}}
   $fixes.Add('displacement='+$target+'-(long)(p+'+($pos+5)+');memcpy(p+'+($pos+1)+',&displacement,4);')
  }
  foreach($operand in $row.operands){
   foreach($match in [regex]::Matches($operand,'0x([0-9a-fA-F]+)')){
    $address=[Convert]::ToInt32($match.Groups[1].Value,16)
    if($row.mnemonic-eq'CALL'-or($row.mnemonic-match'^J'-and$operand-notmatch'\[')){continue}
    $target=$null
    if($address-ge0x489000-and$address-lt0x48b4e8){
     if(!$constants.ContainsKey($address)){$constants[$address]=8192+16*$constants.Count;Emit (Raw $address 8) $constants[$address]}
     $target='(unsigned long)(p+'+$constants[$address]+')'
    }elseif($address-eq0x43a904-or($Damage-and$address-eq0x4363cc)){$target='(unsigned long)(p+2112)'}elseif($address-eq0x43a8d4-or($Damage-and$address-eq0x4363a0)){$target='(unsigned long)(p+2048)'}
    elseif($address-eq0x4dfac4){$target='(unsigned long)(p+12288)'}elseif($address-eq0x4dfab8){$target='(unsigned long)(p+12292)'}
    elseif($address-eq0x4dfb28){$target='(unsigned long)(p+12296)'}elseif($address-eq0x4dfb2c){$target='(unsigned long)(p+12300)'}
    elseif($address-eq0x439910){$target='(unsigned long)bh_actor_response_tick'}
    elseif($Damage-and$address-eq0x4357d0){$target='(unsigned long)bh_actor_damage_tick'}
    elseif($Damage-and$address-eq0x435720){$target=if($DamageLifecycle){'(unsigned long)(p+14336)'}else{'(unsigned long)oracle_unavailable'}}
    elseif($address-eq0x4397e0){$target=if($Lifecycle){'(unsigned long)(p+14336)'}else{'(unsigned long)oracle_unavailable'}}
    elseif($address-eq0x47d680){$target='0UL'}
    elseif($address-ge0x400000-and$address-lt$imageEnd){throw ('Unexpected original absolute operand {0:x}'-f$address)}
    if($target){
     $offset=-1;for($j=0;$j-le$bytes.Length-4;++$j){if([BitConverter]::ToInt32($bytes,$j)-eq$address){$offset=$j;break}}
     if($offset-lt0){throw 'Missing operand bytes'}
     $fixes.Add('address='+$target+';memcpy(p+'+($pos+$offset)+',&address,4);')
    }
   }
  }
 }
 if($cursor-ne$end){throw 'Incomplete original body'}
}
$count=if($Damage){11}else{12};$owner=if($Damage){0x435e80}else{0x43a1d0};$end=if($Damage){0x4363a0}else{0x43a8d1}
$tableAt=if($Damage){0x4363a0}else{0x43a8d4};$indexAt=if($Damage){0x4363cc}else{0x43a904}
$table=Raw $tableAt (4*$count)
for($i=0;$i-lt$count;++$i){$target=[BitConverter]::ToInt32($table,4*$i);if($target-lt$owner-or$target-ge$end){throw 'Foreign switch destination'}}
$indices=Raw $indexAt 21
foreach($index in $indices){if($index-ge$count){throw 'Invalid switch index'}}
Emit $table 2048;Emit $indices 2112
$header.AddRange($fixes)
$header.Add('for(i=0;i<'+$count+';++i){memcpy(&old,p+2048+i*4,4);address=(unsigned long)p+old-'+$owner+';memcpy(p+2048+i*4,&address,4);}}')
[IO.File]::WriteAllLines("$out/original-query.h",$header)
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC';$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH";$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($name in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($name,$null,'Process')}
$build=Get-Content "$root/build/game/build.json" -Raw|ConvertFrom-Json
if(!$build.passed-or$build.debugging){throw 'Current release game objects required'}
foreach($source in @('src/bh_actor_response.c','src/bh_actor_damage.c','src/bh_actor_vehicle_lifetime.c','src/bh_entities.c','src/bh_camera_curve.c','src/bh_target_selection.c','src/bh_sound_queue.c')){if((Get-FileHash "$root/$source").Hash.ToLower()-ne$build.sources.$source){throw 'Rebuild changed production source first'}}
if((Get-FileHash "$root/src/bh_actor_vehicle_release_core.inc").Hash.ToLower()-ne$build.sources.'src/bh_actor_vehicle_release_core.inc'){throw 'Rebuild changed vehicle release core first'}
if((Get-FileHash "$root/src/bh_actor_damage_query_core.inc").Hash.ToLower()-ne$build.sources.'src/bh_actor_damage_query_core.inc'){throw 'Rebuild changed damage query core first'}
if((Get-FileHash "$root/src/bh_actor_damage_tick_core.inc").Hash.ToLower()-ne$build.sources.'src/bh_actor_damage_tick_core.inc'){throw 'Rebuild changed damage tick core first'}
if((Get-FileHash "$root/src/bh_actor_response_query_core.inc").Hash.ToLower()-ne$build.sources.'src/bh_actor_response_query_core.inc'){throw 'Rebuild changed response query core first'}
if((Get-FileHash "$root/src/bh_actor_response_motion.inc").Hash.ToLower()-ne$build.sources.'src/bh_actor_response_motion.inc'){throw 'Rebuild changed response motion first'}
if((Get-FileHash "$root/src/bh_actor_response_smoke.inc").Hash.ToLower()-ne$build.sources.'src/bh_actor_response_smoke.inc'){throw 'Rebuild changed response smoke first'}
if((Get-FileHash "$root/src/bh_actor_response_finish.inc").Hash.ToLower()-ne$build.sources.'src/bh_actor_response_finish.inc'){throw 'Rebuild changed response finish first'}
$cache=Get-Content "$root/build/game/objects.json" -Raw|ConvertFrom-Json
if((Get-FileHash "$root/build/game/bh_actor_vehicle_lifetime.obj").Hash.ToLower()-ne$cache.'bh_actor_vehicle_lifetime.c'.object_sha256){throw 'Vehicle lifetime object differs from production cache'}
$test=if($DamageLifecycle){'ai_damage_lifecycle_original'}elseif($Lifecycle){'ai_query_lifecycle_original'}elseif($Damage){'ai_damage_query_original'}else{'ai_query_original'}
& "$root/analysis/toolchain-sp3/bin/CL.EXE" /nologo /c /ML /O2 /G5 /Oi /Gy /ID:/Decomp/dx7sdk/include /DDIRECT3D_VERSION=0x0500 /DDIRECTDRAW_VERSION=0x0500 "/Fo$out/$test.obj" "$root/tests/$test.c"
if($LASTEXITCODE){throw 'Compile failed'}
$rsp=@($build.commands[-1].argv|Select-Object -Skip 1|Where-Object{$_-notmatch'bh_game_main.obj$|/out:|/map:|/subsystem:'}|ForEach-Object{'"'+$_+'"'})
$rsp+='/subsystem:console';$rsp+=('"'+$out+'/'+$test+'.obj"');$rsp+=('"/out:'+$out+'/'+$test+'.exe"');$rsp+=('"/map:'+$out+'/'+$test+'.map"')
[IO.File]::WriteAllLines("$out/link.rsp",$rsp)
& "$vc/bin/LINK.EXE" "@$out/link.rsp";if($LASTEXITCODE){throw 'Link failed'}
$log=if($DamageLifecycle){'damage-lifecycle-result.log'}elseif($Lifecycle){'lifecycle-result.log'}else{'result.log'}
& "$out/$test.exe"|Tee-Object -FilePath "$out/$log"
exit $LASTEXITCODE
