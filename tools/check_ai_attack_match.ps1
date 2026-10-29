$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent;$out=Join-Path $root 'build/ai-attack-match';New-Item -ItemType Directory -Force $out|Out-Null
$reference=Join-Path $root 'GAME_DATA/BHUNTER.EXE';if((Get-FileHash $reference).Hash-ne'C86205E299119A66E27F687679EE7E978F26FA9A6C0BFB21F664AA6E74E848B1'){throw 'Reference differs'}
$image=[IO.File]::ReadAllBytes($reference);$pe=[BitConverter]::ToInt32($image,0x3c);$sectionAt=$pe+24+[BitConverter]::ToUInt16($image,$pe+20)
$sections=@(for($i=0;$i-lt[BitConverter]::ToUInt16($image,$pe+6);++$i){$at=$sectionAt+40*$i;@{rva=[BitConverter]::ToUInt32($image,$at+12);size=[BitConverter]::ToUInt32($image,$at+16);disk=[BitConverter]::ToUInt32($image,$at+20)}})
function Raw([int]$a,[int]$n){foreach($section in $sections){$offset=$a-0x400000-$section.rva;if($offset-ge0-and$offset+$n-le$section.size){$offset+=$section.disk;return ,([byte[]]$image[$offset..($offset+$n-1)])}};throw 'Unmapped original file range'}
$header=[Collections.Generic.List[string]]::new();$fixes=[Collections.Generic.List[string]]::new();$constants=@{}
$globals=@{0x4dfab8=16384;0x4ce448=16400;0x4cdc70=16416;0x496be0=16432;0x4d0480=16448;0x4f0030=16464;0x4f0028=16480;0x4f0060=16496;0x4f54fc=16512;0x4f5564=16528;0x4f0090=16544;0x4d1894=16560;0x495e30=16640;0x48c070=16768}
$header.Add('static void install_original_attack(unsigned char *p){unsigned long address;long displacement;')
function Emit([byte[]]$bytes,[int]$at){$header.Add('{unsigned char bytes[]={'+($bytes-join',')+'};memcpy(p+'+$at+',bytes,sizeof(bytes));}')}
$attack=Get-Content "$root/META/m4/target600/b7/original/00437ae0.disasm.json" -Raw|ConvertFrom-Json
$curve=Get-Content "$root/META/m4/playable/ai-motion/curve-original-asm.json" -Raw|ConvertFrom-Json
$normal=Get-Content "$root/META/m4/playable/ai-controls/normalize-asm.json" -Raw|ConvertFrom-Json
foreach($range in @(@(0x437ae0,0x438fe3,0,$attack),@(0x4341a0,0x4343bf,6144,$curve),@(0x4011f0,0x401261,8192,$normal))){
 $start,$end,$destination,$rows=$range;Emit (Raw $start ($end-$start)) $destination;$cursor=$start
 foreach($row in $rows){
  $at=[Convert]::ToInt32($row.address,16);if($at-lt$start-or$at-ge$end){continue};$bytes=[Convert]::FromHexString($row.bytes)
  if($cursor-ne$at-or[Convert]::ToHexString((Raw $at $bytes.Length))-ne$row.bytes.ToUpper()){throw 'Original attack instruction gap/mismatch'};$cursor+=$bytes.Length;$pos=$destination+$at-$start
  if($row.mnemonic-eq'CALL'-and$row.operands[0]-match'^0x'){
   $callee=[Convert]::ToInt32($row.operands[0].Substring(2),16)
   $target=switch($callee){0x4340f0{'(long)oracle_clamp'}0x4341a0{'(long)(p+6144)'}0x4011f0{'(long)(p+8192)'}0x4011a0{'(long)(p+8448)'}0x47ddf0{'(long)oracle_rand_original'}0x47bd40{'(long)oracle_sound_original'}0x442360{'(long)oracle_area_original'}0x4069d0{'(long)oracle_split_original'}0x406800{'(long)oracle_orbit_original'}0x4409d0{'(long)oracle_homing_original'}0x4408e0{'(long)oracle_ballistic_original'}0x478540{'(long)oracle_flare_original'}0x4025c0{'(long)oracle_particle_original'}0x478390{'(long)oracle_segment_original'}0x40c4a0{'(long)oracle_world_original'}0x476f20{if($at-eq0x43865f){'(long)(p+18000)'}elseif($at-eq0x438843){'(long)(p+18100)'}else{throw 'Unreviewed inline effect allocation'}}0x407510{'(long)oracle_spark_original'}0x407b40{'(long)oracle_marker_original'}0x403120{'(long)oracle_debris_original'}default{throw 'Unreviewed attack dependency'}}
   $fixes.Add('displacement='+$target+'-(long)(p+'+($pos+5)+');memcpy(p+'+($pos+1)+',&displacement,4);')
  }
  foreach($operand in $row.operands){if($operand-notmatch'\['){continue};foreach($match in [regex]::Matches($operand,'0x([0-9a-fA-F]+)')){
   $address=[Convert]::ToInt32($match.Groups[1].Value,16);if($address-lt0x400000){continue}
   if($globals.ContainsKey($address)){$slot=$globals[$address]}elseif($address-ge0x489000-and$address-lt0x48b4e8){if(!$constants.ContainsKey($address)){$constants[$address]=12288+16*$constants.Count;Emit (Raw $address 8) $constants[$address]};$slot=$constants[$address]}else{throw ('Unreviewed attack absolute operand {0:x}'-f$address)}
   $offset=-1;for($j=0;$j-le$bytes.Length-4;++$j){if([BitConverter]::ToInt32($bytes,$j)-eq$address){$offset=$j;break}};if($offset-lt0){throw 'Missing absolute operand'};$fixes.Add('address=(unsigned long)(p+'+$slot+');memcpy(p+'+($pos+$offset)+',&address,4);')
  }}
  if($row.mnemonic-eq'PUSH'-and$row.operands[0]-in@('0x48c070','0x495e30')){$address=[Convert]::ToInt32($row.operands[0].Substring(2),16);$slot=$globals[$address];$fixes.Add('address=(unsigned long)(p+'+$slot+');memcpy(p+'+($pos+1)+',&address,4);')}
  if($row.mnemonic-eq'MOV'-and$row.operands[-1]-in@('0x403640','0x4035e0','0x407ad0','0x407720')){$target=switch($row.operands[-1]){'0x403640'{'bh_script_flash_tick'}'0x4035e0'{'bh_flash_flare_refresh'}default{'oracle_unavailable'}};if($bytes[0]-ne0xc7){throw 'Inline callback opcode differs'};$fixes.Add('address=(unsigned long)'+$target+';memcpy(p+'+($pos+6)+',&address,4);')}
 }
 if($cursor-ne$end){throw 'Incomplete original attack/math body'}
}
$cross=Raw 0x4011a0 67;$control=Get-Content "$root/META/m4/playable/ai-byte-match/positive-control.json" -Raw|ConvertFrom-Json;if(!$control.matched-or[Convert]::ToHexString($cross).ToLower()-ne$control.original_hex){throw 'Cross positive control differs'};Emit $cross 8448
$bounds=Raw 0x4340f0 169;$control=Get-Content "$root/META/m4/playable/ai-byte-match/game-bounds-object.json" -Raw|ConvertFrom-Json;if(!$control.matched-or[Convert]::ToHexString($bounds).ToLower()-ne$control.original_hex){throw 'Bounds positive control differs'};Emit $bounds 8704
$up=Raw 0x48c070 12;if([BitConverter]::ToSingle($up,0)-ne0-or[BitConverter]::ToSingle($up,4)-ne1-or[BitConverter]::ToSingle($up,8)-ne0){throw 'Original up vector differs'};Emit $up 16768
# Original allocator call-site adapters expose already-computed point/normal
# to the real source impact/flash owners, adding their source-private bindings.
Emit ([Convert]::FromHexString('8d8424e40000008d5424745052e80000000083c408c3')) 18000
$fixes.Add('displacement=(long)oracle_impact_prepare-(long)(p+18018);memcpy(p+18014,&displacement,4);')
Emit ([Convert]::FromHexString('8d44246850e80000000083c404c3')) 18100
$fixes.Add('displacement=(long)oracle_flash_prepare-(long)(p+18110);memcpy(p+18106,&displacement,4);')
# Explicit input for the original unwritten contact-severity stack word.
# Entry ESP-8 is original frame+154 after SUB14c and four saved registers.
Emit ([Convert]::FromHexString('c74424f80000003ee900000000')) 19000
$fixes.Add('displacement=-(19000+13);memcpy(p+19009,&displacement,4);')
$header.AddRange($fixes);$header.Add('memset(p+16640,0,12);}')
[IO.File]::WriteAllLines("$out/original-attack.h",$header)
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC';$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc';$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH";$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($name in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($name,$null,'Process')}
$build=Get-Content "$root/build/game/build.json" -Raw|ConvertFrom-Json;if(!$build.passed-or$build.debugging){throw 'Current release build required'}
if((Get-FileHash "$root/src/bh_actor_attack.c").Hash.ToLower()-ne$build.sources.'src/bh_actor_attack.c'){throw 'Rebuild changed attack source first'}
$flags=@('/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/ID:/Decomp/dx7sdk/include','/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500')
& "$root/analysis/toolchain-sp3/bin/CL.EXE" @flags "/Fo$out/ai_attack_original.obj" "$root/tests/ai_attack_original.c";if($LASTEXITCODE){throw 'Native compile failed'}
$aliases=@('/Drand=oracle_rand_source','/Dbh_sound_request=oracle_sound_source','/Dbh_effect_orbit_create=oracle_orbit_source','/Dbh_effect_split_create=oracle_split_source','/Dbh_area_projectile_create=oracle_area_source','/Dbh_projectile_create_homing=oracle_homing_source','/Dbh_projectile_create_ballistic=oracle_ballistic_source')
$aliases+=@('/Dbh_flare_queue=oracle_flare_source','/Dbh_script_particle_create=oracle_particle_source','/Dbh_draw_segment=oracle_segment_source','/Dbh_world_segment_run=oracle_world_source')
$aliases+=@('/Dbh_contact_spark_create=oracle_spark_source','/Dbh_hit_marker_create=oracle_marker_source','/Dbh_script_debris_create=oracle_debris_source','/Dbh_flash_flare_bind=oracle_flash_source','/Dbh_contact_impact_create=oracle_impact_source')
& "$root/analysis/toolchain-sp3/bin/CL.EXE" @flags @aliases "/Fo$out/attack-observed.obj" "$root/src/bh_actor_attack.c";if($LASTEXITCODE){throw 'Observed real attack compile failed'}
$rsp=@($build.commands[-1].argv|Select-Object -Skip 1|Where-Object{$_-notmatch'bh_game_main.obj$|bh_actor_attack.obj$|/out:|/map:|/subsystem:'}|ForEach-Object{'"'+$_+'"'});$rsp+='/subsystem:console';$rsp+=('"'+$out+'/ai_attack_original.obj"');$rsp+=('"'+$out+'/attack-observed.obj"');$rsp+=('"/out:'+$out+'/ai-attack-match.exe"');$rsp+=('"/map:'+$out+'/ai-attack-match.map"');[IO.File]::WriteAllLines("$out/link.rsp",$rsp)
& "$vc/bin/LINK.EXE" "@$out/link.rsp";if($LASTEXITCODE){throw 'Native link failed'}
& "$out/ai-attack-match.exe"|Tee-Object -FilePath "$out/result.log";exit $LASTEXITCODE
