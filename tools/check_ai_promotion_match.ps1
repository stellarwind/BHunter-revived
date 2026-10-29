param([switch]$Pursuit, [switch]$DueClock)
$ErrorActionPreference='Stop'
if($Pursuit-and$DueClock){throw 'Separate pursuit and due-clock comparison variants required'}
$root=Split-Path $PSScriptRoot -Parent;$out=Join-Path $root 'build/ai-promotion-match'
New-Item -ItemType Directory -Force $out|Out-Null
$reference=Join-Path $root 'GAME_DATA/BHUNTER.EXE'
if((Get-FileHash $reference).Hash-ne'C86205E299119A66E27F687679EE7E978F26FA9A6C0BFB21F664AA6E74E848B1'){throw 'Reference differs'}
$image=[IO.File]::ReadAllBytes($reference)
$pe=[BitConverter]::ToInt32($image,0x3c);$sectionAt=$pe+24+[BitConverter]::ToUInt16($image,$pe+20)
$sections=@(for($i=0;$i-lt[BitConverter]::ToUInt16($image,$pe+6);++$i){$at=$sectionAt+40*$i;@{rva=[BitConverter]::ToUInt32($image,$at+12);size=[BitConverter]::ToUInt32($image,$at+16);disk=[BitConverter]::ToUInt32($image,$at+20)}})
function Raw([int]$a,[int]$n){foreach($section in $sections){$offset=$a-0x400000-$section.rva;if($offset-ge0-and$offset+$n-le$section.size){$offset+=$section.disk;return ,([byte[]]$image[$offset..($offset+$n-1)])}};throw 'Unmapped original file range'}
$header=[Collections.Generic.List[string]]::new();$fixes=[Collections.Generic.List[string]]::new();$constants=@{}
$header.Add('static void install_original_promotion(unsigned char *p){unsigned long address,old;long displacement;int i;')
function Emit([byte[]]$bytes,[int]$at){$header.Add('{unsigned char bytes[]={'+($bytes-join',')+'};memcpy(p+'+$at+',bytes,sizeof(bytes));}')}
$promotion=@(Import-Csv "$root/META/m4/target-steering/original/00434f40.tsv" -Delimiter "`t"|ForEach-Object{$parts=$_.instruction-split' ',2;[pscustomobject]@{address=$_.address;bytes=$_.bytes;mnemonic=$parts[0];operands=@($parts[1]-split',')}})
$advance=@(Import-Csv "$root/META/m4/target-steering/original/004355c0.tsv" -Delimiter "`t"|ForEach-Object{$parts=$_.instruction-split' ',2;[pscustomobject]@{address=$_.address;bytes=$_.bytes;mnemonic=$parts[0];operands=@($parts[1]-split',')}})
$choice=@(Import-Csv "$root/META/m4/target-steering/original/004344f0.tsv" -Delimiter "`t"|ForEach-Object{$parts=$_.instruction-split' ',2;[pscustomobject]@{address=$_.address;bytes=$_.bytes;mnemonic=$parts[0];operands=@($parts[1]-split',')}})
$due=@(Import-Csv "$root/META/m4/target-steering/original/00435790.tsv" -Delimiter "`t"|ForEach-Object{$parts=$_.instruction-split' ',2;[pscustomobject]@{address=$_.address;bytes=$_.bytes;mnemonic=$parts[0];operands=@($parts[1]-split',')}})
$release=@(Import-Csv "$root/META/m4/target-steering/original/00435720.tsv" -Delimiter "`t"|ForEach-Object{$parts=$_.instruction-split' ',2;[pscustomobject]@{address=$_.address;bytes=$_.bytes;mnemonic=$parts[0];operands=@($parts[1]-split',')}})
$guided=Get-Content "$root/META/m4/playable/ai-byte-match/00434650-asm.json" -Raw|ConvertFrom-Json
$portal=Get-Content "$root/META/m4/playable/ai-byte-match/00433f70-asm.json" -Raw|ConvertFrom-Json
$axis=Get-Content "$root/META/m4/playable/ai-byte-match/004343c0-asm.json" -Raw|ConvertFrom-Json
$ftol=Get-Content "$root/META/m4/playable/ai-controls/ftol-asm.json" -Raw|ConvertFrom-Json
$ranges=@(@(0x434f40,0x435580,0,$promotion),@(0x4343c0,0x4344ea,4096,$axis),@(0x47de20,0x47de47,6144,$ftol),@(0x4355c0,0x43571e,16384,$advance),@(0x434650,0x434808,17408,$guided),@(0x4344f0,0x434648,18432,$choice),@(0x433f70,0x434015,19456,$portal),@(0x435790,0x4357ca,22528,$due),@(0x435720,0x435786,23040,$release))
if($Pursuit){
 $pursuitRows=Get-Content "$root/META/m4/target600/b4/original/00438ff0.disasm.json" -Raw|ConvertFrom-Json
 $target=Get-Content "$root/META/m4/playable/ai-aggression/00421610-asm.json" -Raw|ConvertFrom-Json
 $length=Get-Content "$root/META/m4/playable/ai-controls/length-asm.json" -Raw|ConvertFrom-Json
 $curve=Get-Content "$root/META/m4/playable/ai-motion/curve-original-asm.json" -Raw|ConvertFrom-Json
 $ranges+=,@(0x438ff0,0x4397cf,28672,$pursuitRows);$ranges+=,@(0x421610,0x421740,2560,$target);$ranges+=,@(0x434810,0x434916,27008,$length);$ranges+=,@(0x4341a0,0x4343bf,27648,$curve)
}
foreach($range in $ranges){
 $start,$end,$destination,$rows=$range;Emit (Raw $start ($end-$start)) $destination;$cursor=$start
 foreach($row in $rows){
  $at=[Convert]::ToInt32($row.address,16);if($at-lt$start-or$at-ge$end){continue};$bytes=[Convert]::FromHexString($row.bytes)
  if($cursor-ne$at-or[Convert]::ToHexString((Raw $at $bytes.Length))-ne$row.bytes.ToUpper()){throw 'Original instruction gap/mismatch'};$cursor+=$bytes.Length;$pos=$destination+$at-$start
  if($row.mnemonic-eq'CALL'-and$row.operands[0]-match'^0x'){
   $target=switch([Convert]::ToInt32($row.operands[0].Substring(2),16)){0x4343c0{'(long)(p+4096)'}0x47de20{'(long)(p+6144)'}0x434f40{'(long)p'}0x434650{'(long)(p+17408)'}0x4344f0{'(long)(p+18432)'}0x4355c0{if($DueClock){ '(long)oracle_due_advance_original' }else{'(long)(p+16384)'}}0x433f70{'(long)(p+19456)'}0x434040{'(long)(p+20480)'}0x4011a0{'(long)(p+21504)'}0x47c140{if($Pursuit){'(long)oracle_sound_stop'}else{'(long)oracle_sound_stop_original'}}0x477070{'(long)oracle_release'}0x47ddf0{'(long)oracle_rand_original'}0x421610{'(long)(p+2560)'}0x4340f0{'(long)(p+26624)'}0x434810{'(long)(p+27008)'}0x4341a0{'(long)(p+27648)'}0x47dc30{'(long)sprintf'}0x442a30{'(long)oracle_message'}default{throw 'Unexpected promotion/advance/pursuit call'}}
   $fixes.Add('displacement='+$target+'-(long)(p+'+($pos+5)+');memcpy(p+'+($pos+1)+',&displacement,4);')
  }
  foreach($operand in $row.operands){if($operand-notmatch'\['){continue};foreach($match in [regex]::Matches($operand,'0x([0-9a-fA-F]+)')){
   $address=[Convert]::ToInt32($match.Groups[1].Value,16);if($address-lt0x400000){continue}
   if($address-eq0x435580){$slot=2048}elseif($address-eq0x43559c){$slot=2080}elseif($address-eq0x4dfac4){$slot=12288}
   elseif($address-eq0x434018){$slot=19712}elseif($address-eq0x4340d4){$slot=20736}elseif($address-ge0x497230-and$address-lt0x4973a4){$slot=24576+$address-0x497230}
   elseif($address-ge0x48c070-and$address-le0x48c078){$slot=25088+$address-0x48c070}
   elseif($Pursuit-and$address-eq0x4397d0){$slot=31104}
   elseif($Pursuit-and$address-eq0x4dfab8){$slot=12292}elseif($Pursuit-and$address-eq0x4dfb28){$slot=12296}
   elseif($Pursuit-and$address-eq0x4df264){$slot=12300}elseif($Pursuit-and$address-eq0x4df260){$slot=12304}
   elseif($Pursuit-and$address-eq0x4f556c){$slot=12308}elseif($Pursuit-and$address-eq0x48fb50){$slot=12312}
   elseif($Pursuit-and$address-eq0x48fb58){$slot=12320}
   elseif($address-ge0x489000-and$address-lt0x48b4e8){if(!$constants.ContainsKey($address)){$constants[$address]=8192+16*$constants.Count;Emit (Raw $address 8) $constants[$address]};$slot=$constants[$address]}
   else{throw ('Unexpected promotion absolute operand {0:x}'-f$address)}
   $offset=-1;for($j=0;$j-le$bytes.Length-4;++$j){if([BitConverter]::ToInt32($bytes,$j)-eq$address){$offset=$j;break}};if($offset-lt0){throw 'Missing operand'}
   $fixes.Add('address=(unsigned long)(p+'+$slot+');memcpy(p+'+($pos+$offset)+',&address,4);')
  }}
  if($row.mnemonic-eq'PUSH'-and$row.operands[0]-eq'0x48c070'){$fixes.Add('address=(unsigned long)(p+25088);memcpy(p+'+($pos+1)+',&address,4);')}
  if($Pursuit-and$row.mnemonic-eq'PUSH'-and$row.operands[0]-in@('0x490568','0x49052c','0x4904f4','0x4904a4')){
   $address=[Convert]::ToInt32($row.operands[0].Substring(2),16);$slot=switch($address){0x490568{31400}0x49052c{31600}0x4904f4{31800}0x4904a4{32000}}
   $string=Raw $address 200;$endIndex=[Array]::IndexOf($string,[byte]0);if($endIndex-lt0-or$bytes[0]-ne0x68){throw 'Original report string boundary differs'};Emit ([byte[]]$string[0..$endIndex]) $slot
   $fixes.Add('address=(unsigned long)(p+'+$slot+');memcpy(p+'+($pos+1)+',&address,4);')
  }
 }
 if($cursor-ne$end){throw ('Incomplete original body {0:x}: cursor {1:x} expected {2:x}'-f$start,$cursor,$end)}
}
foreach($table in @(@(0x435580,2048),@(0x43559c,2080))){$address,$destination=$table;$bytes=Raw $address 28;for($i=0;$i-lt7;++$i){$target=[BitConverter]::ToInt32($bytes,4*$i);if($target-lt0x434f40-or$target-ge0x435580){throw 'Foreign promotion switch target'}};Emit $bytes $destination;$fixes.Add('for(i=0;i<7;++i){memcpy(&old,p+'+$destination+'+i*4,4);address=(unsigned long)p+old-0x434f40;memcpy(p+'+$destination+'+i*4,&address,4);}')}
foreach($table in @(@(0x434018,19712,0x433f70,0x434015,19456),@(0x4340d4,20736,0x434040,0x4340d3,20480))){$address,$destination,$owner,$end,$base=$table;$bytes=Raw $address 28;for($i=0;$i-lt7;++$i){$target=[BitConverter]::ToInt32($bytes,4*$i);if($target-lt$owner-or$target-ge$end){throw 'Foreign portal switch target'}};Emit $bytes $destination;$fixes.Add('for(i=0;i<7;++i){memcpy(&old,p+'+$destination+'+i*4,4);address=(unsigned long)p+'+$base+'+old-'+$owner+';memcpy(p+'+$destination+'+i*4,&address,4);}')}
$cross=Raw 0x4011a0 67;$positive=Get-Content "$root/META/m4/playable/ai-byte-match/positive-control.json" -Raw|ConvertFrom-Json
if(!$positive.matched-or[Convert]::ToHexString($cross).ToLower()-ne$positive.original_hex){throw 'Cross-product positive control differs'};Emit $cross 21504
$normal=Raw 0x434040 147;$normalControl=Get-Content "$root/META/m4/playable/ai-byte-match/game-normal-object.json" -Raw|ConvertFrom-Json
if(!$normalControl.matched-or[Convert]::ToHexString($normal).ToLower()-ne$normalControl.original_hex-or[BitConverter]::ToInt32($normal,29)-ne0x4340d4){throw 'Complete normal/table positive control differs'};Emit $normal 20480
$fixes.Add('address=(unsigned long)(p+20736);memcpy(p+20509,&address,4);')
# Initialized .data vector, outside the read-only constants section.
$up=[byte[]]$image[0x8a070..0x8a07b]
if([BitConverter]::ToSingle($up,0)-ne0-or[BitConverter]::ToSingle($up,4)-ne1-or[BitConverter]::ToSingle($up,8)-ne0){throw 'Original up vector differs'};Emit $up 25088
if($Pursuit){
 $bounds=Raw 0x4340f0 169;$control=Get-Content "$root/META/m4/playable/ai-byte-match/game-bounds-object.json" -Raw|ConvertFrom-Json
 if(!$control.matched-or[Convert]::ToHexString($bounds).ToLower()-ne$control.original_hex){throw 'Complete bounds positive control differs'};Emit $bounds 26624
 $table=Raw 0x4397d0 16;for($i=0;$i-lt4;++$i){$target=[BitConverter]::ToInt32($table,4*$i);if($target-lt0x438ff0-or$target-ge0x4397cf){throw 'Foreign pursuit report switch target'}};Emit $table 31104
 $fixes.Add('for(i=0;i<4;++i){memcpy(&old,p+31104+i*4,4);address=(unsigned long)p+28672+old-0x438ff0;memcpy(p+31104+i*4,&address,4);}')
}
$header.AddRange($fixes);$header.Add('}');[IO.File]::WriteAllLines("$out/original-promotion.h",$header)
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC';$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH";$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($name in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($name,$null,'Process')}
$build=Get-Content "$root/build/game/build.json" -Raw|ConvertFrom-Json
if(!$build.passed-or$build.debugging){throw 'Current release build required'}
foreach($source in @('src/bh_actor_route.c','src/bh_actor_route_lifetime.c','src/bh_axis_snap.c','src/bh_route_guided.c','src/bh_route_choice.c','src/bh_route_query.c','src/bh_actor_pursuit_route.c','src/bh_actor_response.c','src/bh_target_selection.c')){if((Get-FileHash "$root/$source").Hash.ToLower()-ne$build.sources.$source){throw 'Rebuild changed production source first'}}
foreach($source in @('src/bh_actor_route_promote_core.inc','src/bh_actor_route_due_core.inc','src/bh_actor_route_release_core.inc','src/bh_actor_route_advance_core.inc','src/bh_actor_guided_exit_core.inc','src/bh_actor_portal_center_core.inc','src/bh_actor_portal_core.inc','src/bh_actor_exit_core.inc')){if((Get-FileHash "$root/$source").Hash.ToLower()-ne$build.sources.$source){throw 'Rebuild changed included source first'}}
$cache=Get-Content "$root/build/game/objects.json" -Raw|ConvertFrom-Json
foreach($name in @('bh_actor_route_lifetime','bh_actor_route','bh_route_guided','bh_route_query','bh_route_choice')){if((Get-FileHash "$root/build/game/$name.obj").Hash.ToLower()-ne$cache."$name.c".object_sha256){throw 'Production route object hash differs'}}
$testFlags=@();if($DueClock){$testFlags+='/DBH_DUE_CLOCK_CHECK'}
$test=if($Pursuit){'ai_pursuit_original'}else{'ai_promotion_original'}
& "$root/analysis/toolchain-sp3/bin/CL.EXE" /nologo /c /ML /O2 /G5 /Oi /Gy /ID:/Decomp/dx7sdk/include /DDIRECT3D_VERSION=0x0500 /DDIRECTDRAW_VERSION=0x0500 @testFlags "/Fo$out/$test.obj" "$root/tests/$test.c";if($LASTEXITCODE){throw 'Native compile failed'}
$rsp=@($build.commands[-1].argv|Select-Object -Skip 1|Where-Object{$_-notmatch'bh_game_main.obj$|/out:|/map:|/subsystem:'}|ForEach-Object{'"'+$_+'"'})
if($Pursuit){
 & "$root/analysis/toolchain-sp3/bin/CL.EXE" /nologo /c /ML /O2 /G5 /Oi /Gy /Drand=oracle_rand_source /Dbh_sound_stop=oracle_sound_source "/Fo$out/pursuit-counted.obj" "$root/src/bh_actor_pursuit_route.c";if($LASTEXITCODE){throw 'Counted real pursuit compile failed'}
 $rsp=@($rsp|Where-Object{$_-notmatch'bh_actor_pursuit_route.obj'});$rsp+=('"'+$out+'/pursuit-counted.obj"')
}
if($DueClock){
 & "$root/analysis/toolchain-sp3/bin/CL.EXE" /nologo /c /ML /O2 /G5 /Oi /Gy /Dbh_actor_route_advance=oracle_due_advance_source /ID:/Decomp/dx7sdk/include /DDIRECT3D_VERSION=0x0500 /DDIRECTDRAW_VERSION=0x0500 "/Fo$out/due-clock-observed.obj" "$root/src/bh_actor_route_lifetime.c";if($LASTEXITCODE){throw 'Due-clock observed source compile failed'}
 $rsp=@($rsp|Where-Object{$_-notmatch'bh_actor_route_lifetime\.obj"?$'});$rsp+=('"'+$out+'/due-clock-observed.obj"')
 [IO.File]::WriteAllText("$out/due-clock-alias.txt",'/Dbh_actor_route_advance=oracle_due_advance_source')
}
$rsp+='/subsystem:console';$rsp+=('"'+$out+'/'+$test+'.obj"');$rsp+=('"/out:'+$out+'/'+$test+'.exe"');$rsp+=('"/map:'+$out+'/'+$test+'.map"');[IO.File]::WriteAllLines("$out/link.rsp",$rsp)
& "$vc/bin/LINK.EXE" "@$out/link.rsp";if($LASTEXITCODE){throw 'Native link failed'}
$log=if($Pursuit){'pursuit-result.log'}elseif($DueClock){'due-clock-result.log'}else{'result.log'}
& "$out/$test.exe"|Tee-Object -FilePath "$out/$log";exit $LASTEXITCODE
