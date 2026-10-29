param([switch]$WholeTick,[switch]$Damage)
if($Damage){$WholeTick=$true}
# Isolated response tail/whole tick or ordinary damage tick, with real typed owners.
$ErrorActionPreference='Stop'
$ownerStem=if($Damage){'bh_actor_damage'}else{'bh_actor_response'}
$root=Split-Path $PSScriptRoot -Parent;$out=Join-Path $root $(if($Damage){'build/ai-damage-tick-match'}elseif($WholeTick){'build/ai-response-whole-match'}else{'build/ai-response-finish-match'})
New-Item -ItemType Directory -Force $out|Out-Null
$reference=Join-Path $root 'GAME_DATA/BHUNTER.EXE'
if((Get-FileHash $reference).Hash-ne'C86205E299119A66E27F687679EE7E978F26FA9A6C0BFB21F664AA6E74E848B1'){throw 'Reference differs'}
$image=[IO.File]::ReadAllBytes($reference)
function Raw([int]$a,[int]$n){$offset=if($a-lt0x489000){$a-0x400c00}else{$a-0x489000+555520};return ,([byte[]]$image[$offset..($offset+$n-1)])}
# Preserve original prologue/epilogue slots; seed saved position and explicit legacy angles.
$prefix=[Convert]::FromHexString('81ecac000000535556578bb424c00000008dbe2c0100008d9e500100008dae440100008b8c24c40000008b01894424408b4104894424448b4108894424488b8c24c80000008b01898424b00000008b4104898424b40000008b4108898424b8000000')
if($WholeTick){$prefix=[Convert]::FromHexString('8b54240c8b4c24048b02894424ec8b4204894424f08b4208894424f451e80000000083c404c3')}
$header=[Collections.Generic.List[string]]::new();$fixes=[Collections.Generic.List[string]]::new();$constants=@{}
$header.Add('static void install_original_finish(unsigned char *p){unsigned long address;long displacement;')
function Emit([byte[]]$bytes,[int]$at){$header.Add('{unsigned char bytes[]={'+($bytes-join',')+'};memcpy(p+'+$at+',bytes,sizeof(bytes));}')}
Emit $prefix 0
$healthyAt=if($WholeTick){3072}else{2048}
if($WholeTick){$call=[Array]::IndexOf($prefix,[byte]0xe8);if($call-lt0){throw 'Missing seed shim call'};$fixes.Add('displacement=(long)(p+256)-(long)(p+'+($call+5)+');memcpy(p+'+($call+1)+',&displacement,4);');Emit (Raw 0x47de20 39) 6144}
$healthyPrefix=[Convert]::FromHexString('81ecac000000535556578bb424c0000000bf0000803f');if($Damage){$healthyPrefix=[Convert]::FromHexString('81ec84000000535556578bb42498000000bf0000803f')}
Emit $healthyPrefix $healthyAt
$ranges=@(@(0x439de7,0x43a18a,$prefix.Length,'META/m4/target656/response/original/00439910.disasm.json'),@(0x43a18f,0x43a1c7,($healthyAt+$healthyPrefix.Length),'META/m4/target656/response/original/00439910.disasm.json'),@(0x4341a0,0x4343bf,4096,'META/m4/playable/ai-motion/curve-original-asm.json'))
if($WholeTick){$ranges[0]=@(0x439910,0x43a1c7,256,'META/m4/target656/response/original/00439910.disasm.json')}
if($Damage){$ranges[0]=@(0x4357d0,0x435e7d,256,'META/m4/target600/b3/original/004357d0.disasm.json');$ranges[1]=@(0x435e3e,0x435e7d,($healthyAt+$healthyPrefix.Length),'META/m4/target600/b3/original/004357d0.disasm.json')}
foreach($range in $ranges){
 $begin,$finish,$destination,$file=$range;Emit (Raw $begin ($finish-$begin)) $destination;$cursor=$begin
 foreach($row in(Get-Content "$root/$file" -Raw|ConvertFrom-Json)){
  $at=[Convert]::ToInt32($row.address,16);if($at-lt$begin-or$at-ge$finish){continue}
  $bytes=[Convert]::FromHexString($row.bytes)
  if($at-ne$cursor-or[Convert]::ToHexString((Raw $at $bytes.Length))-ne$row.bytes.ToUpper()){throw 'Original instruction gap/mismatch'};$cursor+=$bytes.Length
  $pos=$destination+$at-$begin
  if($row.mnemonic-eq'CALL'){
   $target=switch($row.operands[0]){'0x004355c0'{'oracle_advance'}'0x00437ae0'{'oracle_attack'}'0x00438ff0'{'oracle_pursuit'}'0x004025c0'{'oracle_particle'}'0x0047de20'{'(p+6144)'}'0x00476e90'{'oracle_set_query'}'0x0047c140'{'oracle_stop'}'0x004341a0'{'tail_curve'}'0x0047ddf0'{'tail_rand'}'0x00403120'{'oracle_debris'}'0x0047bd40'{'oracle_sound'}'0x00423400'{'oracle_loot'}'0x00476f20'{'oracle_acquire'}'0x0043d7c0'{'oracle_construct'}'0x004770f0'{'oracle_broadcast'}'0x00477070'{'oracle_release'}default{throw 'Unreviewed call'}}
   $fixes.Add('displacement=(long)'+$target+'-(long)(p+'+($pos+5)+');memcpy(p+'+($pos+1)+',&displacement,4);');continue
  }
  foreach($operand in $row.operands){foreach($match in [regex]::Matches($operand,'0x([0-9a-fA-F]+)')){
   $address=[Convert]::ToInt32($match.Groups[1].Value,16)
   if($row.mnemonic-match'^J'){if($address-lt$begin-or$address-ge$finish){throw 'Foreign branch'};continue}
   $target=$null
   if($address-ge0x489000-and$address-lt0x48b4e8){
    if(!$constants.ContainsKey($address)){$constants[$address]=12288+16*$constants.Count;Emit (Raw $address 8) $constants[$address]}
    $target='(unsigned long)(p+'+$constants[$address]+')'
   }elseif($address-eq0x4dfb08){$target='(unsigned long)(p+8216)'}elseif($address-eq0x4f0028){$target='(unsigned long)(p+8212)'}elseif($address-eq0x4dfac4){$target='(unsigned long)(p+8208)'}elseif($address-eq0x4d1918){$target='(unsigned long)(p+8192)'}elseif($address-eq0x4f5540){$target='(unsigned long)(p+8196)'}
   elseif($address-eq0x4dfb28){$target='(unsigned long)(p+8200)'}elseif($address-eq0x4dfb2c){$target='(unsigned long)(p+8204)'}
   elseif($address-eq0x435790){$target='(unsigned long)source_normal_step'}elseif($address-eq0x4398c0){$target='(unsigned long)bh_actor_vehicle_tick'}elseif($address-eq0x437a30){$target='(unsigned long)bh_mission_sound_tick'}elseif($address-eq0x47d680){$target='0UL'}
   elseif($address-ge0x400000-and$address-lt0x600000){throw 'Unreviewed absolute operand'}
   if($target){
    $offset=-1;for($j=0;$j-le$bytes.Length-4;++$j){if([BitConverter]::ToInt32($bytes,$j)-eq$address){$offset=$j;break}}
    if($offset-lt0){throw 'Operand bytes absent'}
    $fixes.Add('address='+$target+';memcpy(p+'+($pos+$offset)+',&address,4);')
   }
  }}
 }
 if($cursor-ne$finish){throw 'Incomplete original tail/curve'}
}
$header.AddRange($fixes);$header.Add('}');[IO.File]::WriteAllLines("$out/original-finish.h",$header)
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC';$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH";$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($name in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($name,$null,'Process')}
$build=Get-Content "$root/build/game/build.json" -Raw|ConvertFrom-Json
if(!$build.passed-or$build.debugging){throw 'Current release objects required'}
foreach($source in @('src/bh_actor_damage.c','src/bh_actor_damage_tick_core.inc','src/bh_actor_damage_query_core.inc','src/bh_actor_response.c','src/bh_actor_response_motion.inc','src/bh_actor_response_smoke.inc','src/bh_actor_response_finish.inc','src/bh_actor_response_query_core.inc')){
 if((Get-FileHash "$root/$source").Hash.ToLower()-ne$build.sources.$source){throw 'Rebuild changed production source first'}
}
$flags=@('/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/ID:/Decomp/dx7sdk/include','/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500')
$defines=@(if($WholeTick){'/DRESPONSE_WHOLE_ORIGINAL'};if($Damage){'/DDAMAGE_WHOLE_ORIGINAL';'/Dbh_actor_response_tick=bh_actor_damage_tick'})
& "$root/analysis/toolchain-sp3/bin/CL.EXE" @flags @defines "/Fo$out/ai_response_finish_original.obj" "$root/tests/ai_response_finish_original.c"
if($LASTEXITCODE){throw 'Compile failed'}
$aliases=@('/Drand=tail_rand','/Dbh_entity_set_query=tail_set_query','/Dbh_sound_stop=tail_stop','/Dbh_camera_curve=tail_curve','/Dbh_script_debris_create=tail_debris','/Dbh_sound_request=tail_sound','/Dbh_remote_choose_slot=tail_loot','/Dbh_entity_acquire=tail_acquire','/Dbh_target_construct_moving=tail_construct','/Dbh_entities_query=tail_broadcast','/Dbh_entity_release=tail_release')
& "$root/analysis/toolchain-sp3/bin/CL.EXE" @flags @aliases "/Fo$out/response-observed.obj" "$root/src/$ownerStem.c"
if($LASTEXITCODE){throw 'Observed source compile failed'}
$rsp=@($build.commands[-1].argv|Select-Object -Skip 1|Where-Object{$_-notmatch('bh_game_main.obj$|'+$ownerStem+'.obj$|/out:|/map:|/subsystem:')}|ForEach-Object{'"'+$_+'"'})
$rsp+='/subsystem:console';$rsp+=('"'+$out+'/ai_response_finish_original.obj"');$rsp+=('"'+$out+'/response-observed.obj"');$rsp+=('"/out:'+$out+'/ai-response-finish-match.exe"');$rsp+=('"/map:'+$out+'/ai-response-finish-match.map"');[IO.File]::WriteAllLines("$out/link.rsp",$rsp)
& "$vc/bin/LINK.EXE" "@$out/link.rsp";if($LASTEXITCODE){throw 'Link failed'}
Push-Location $out
try{& "$out/ai-response-finish-match.exe"|Tee-Object "$out/result.log";$result=$LASTEXITCODE}finally{Pop-Location}
exit $result
