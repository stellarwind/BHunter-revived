param([switch]$SourceContract,[string]$CandidateSource)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out=Join-Path $root 'build/ai-pose-match'
New-Item -ItemType Directory -Force $out|Out-Null
$reference=Join-Path $root 'GAME_DATA/BHUNTER.EXE'
if((Get-FileHash $reference).Hash-ne'C86205E299119A66E27F687679EE7E978F26FA9A6C0BFB21F664AA6E74E848B1'){throw 'Reference differs'}
$image=[IO.File]::ReadAllBytes($reference)
$pe=[BitConverter]::ToInt32($image,0x3c);$sectionAt=$pe+24+[BitConverter]::ToUInt16($image,$pe+20)
$sections=@(for($i=0;$i-lt[BitConverter]::ToUInt16($image,$pe+6);++$i){$at=$sectionAt+40*$i;@{rva=[BitConverter]::ToUInt32($image,$at+12);size=[BitConverter]::ToUInt32($image,$at+16);disk=[BitConverter]::ToUInt32($image,$at+20)}})
function Raw([int]$a,[int]$n){foreach($s in $sections){$o=$a-0x400000-$s.rva;if($o-ge0-and$o+$n-le$s.size){$o+=$s.disk;return ,([byte[]]$image[$o..($o+$n-1)])}};throw 'Unmapped reference'}
$header=[Collections.Generic.List[string]]::new();$fixes=[Collections.Generic.List[string]]::new();$constants=@{}
$globals=@{0x4dfac4=8192;0x496334=8200;0x4d1940=8204;0x4f0078=8208;0x4f008c=8212;0x4f556c=8216}
$calls=@{0x4341a0='bh_camera_curve';0x478170='oracle_model';0x47ddf0='rand';0x478280='oracle_billboard';0x401460='bh_mat34_translate_world';0x4017e0='bh_mat34_rotate_y';0x401610='bh_mat34_rotate_x';0x4019a0='bh_mat34_rotate_z';0x4012e0='bh_mat34_transform_point';0x47bd40='oracle_request';0x47c050='oracle_update'}
$header.Add('static void install_original_pose(unsigned char *p){unsigned long address;long displacement;')
function Emit([byte[]]$b,[int]$at){$header.Add('{unsigned char b[]={'+($b-join',')+'};memcpy(p+'+$at+',b,sizeof(b));}')}
Emit (Raw 0x434920 1561) 0
$cursor=0x434920
foreach($row in(Get-Content "$root/META/m4/target600/b/original/00434920.disasm.json" -Raw|ConvertFrom-Json)){
 $at=[Convert]::ToInt32($row.address,16);$b=[Convert]::FromHexString($row.bytes)
 if($at-ne$cursor-or[Convert]::ToHexString((Raw $at $b.Length))-ne$row.bytes.ToUpper()){throw 'Original instruction gap/mismatch'}
 $cursor+=$b.Length;$pos=$at-0x434920
 if($row.mnemonic-eq'CALL'){$target=[Convert]::ToInt32($row.operands[0].Substring(2),16);if(!$calls.ContainsKey($target)){throw 'Unreviewed call'};$fixes.Add('displacement=(long)'+$calls[$target]+'-(long)(p+'+($pos+5)+');memcpy(p+'+($pos+1)+',&displacement,4);')}
 foreach($operand in $row.operands){if($operand-notmatch'\['){continue};foreach($m in [regex]::Matches($operand,'0x([0-9a-fA-F]+)')){
  $a=[Convert]::ToInt32($m.Groups[1].Value,16);if($a-lt0x400000){continue}
  if($globals.ContainsKey($a)){$slot=$globals[$a]}elseif($a-ge0x489000-and$a-lt0x48b4e8){if(!$constants.ContainsKey($a)){$constants[$a]=4096+16*$constants.Count;Emit (Raw $a 8) $constants[$a]};$slot=$constants[$a]}else{throw 'Unreviewed global'}
  $offset=-1;for($j=0;$j-le$b.Length-4;++$j){if([BitConverter]::ToInt32($b,$j)-eq$a){$offset=$j;break}};if($offset-lt0){throw 'Missing operand'}
  $fixes.Add('address=(unsigned long)(p+'+$slot+');memcpy(p+'+($pos+$offset)+',&address,4);')
 }}
}
if($cursor-ne0x434f39){throw 'Incomplete owner'}
$header.AddRange($fixes);$header.Add('}')
[IO.File]::WriteAllLines("$out/original-pose.h",$header)
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC';$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH";$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($n in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($n,$null,'Process')}
$build=Get-Content "$root/build/game/build.json" -Raw|ConvertFrom-Json
if(!$build.passed-or$build.debugging){throw 'Release build required'}
if((Get-FileHash "$root/src/bh_actor_vehicle_draw.c").Hash.ToLower()-ne$build.sources.'src/bh_actor_vehicle_draw.c'){throw 'Rebuild pose source first'}
$flags=@('/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/ID:/Decomp/dx7sdk/include','/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500')
$candidateObject=$null
if($CandidateSource){
 $candidateObject=Join-Path $out 'candidate.obj'
 & "$root/analysis/toolchain-sp3/bin/CL.EXE" @flags "/I$root/src" "/Fo$candidateObject" $CandidateSource;if($LASTEXITCODE){throw 'Candidate compile failed'}
}
$name=if($SourceContract){'actor_vehicle_draw'}else{'ai_pose_original'}
if($SourceContract){$flags+='/DBH_ACTOR_VEHICLE_DRAW_STANDALONE'}
& "$root/analysis/toolchain-sp3/bin/CL.EXE" @flags "/Fo$out/$name.obj" "$root/tests/$name.c";if($LASTEXITCODE){throw 'Compile failed'}
$rsp=@($build.commands[-1].argv|Select-Object -Skip 1|Where-Object{$_-notmatch'bh_game_main.obj$|/out:|/map:|/subsystem:'}|ForEach-Object{'"'+$_+'"'})
if($candidateObject){$rsp=@($rsp|Where-Object{$_-notmatch'bh_actor_vehicle_draw.obj"$'});$rsp+=('"'+$candidateObject+'"')}
$rsp+='/subsystem:console';$rsp+=('"'+$out+'/'+$name+'.obj"');$rsp+=('"/out:'+$out+'/'+$name+'.exe"');$rsp+=('"/map:'+$out+'/'+$name+'.map"');[IO.File]::WriteAllLines("$out/link.rsp",$rsp)
& "$vc/bin/LINK.EXE" "@$out/link.rsp";if($LASTEXITCODE){throw 'Link failed'}
& "$out/$name.exe"|Tee-Object -FilePath "$out/$name.log";exit $LASTEXITCODE

