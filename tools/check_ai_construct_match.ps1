$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent;$out=Join-Path $root 'build/ai-construct-match'
New-Item -ItemType Directory -Force $out|Out-Null
$reference=Join-Path $root 'GAME_DATA/BHUNTER.EXE'
if((Get-FileHash $reference).Hash-ne'C86205E299119A66E27F687679EE7E978F26FA9A6C0BFB21F664AA6E74E848B1'){throw 'Reference differs'}
$image=[IO.File]::ReadAllBytes($reference);$optional=[BitConverter]::ToInt32($image,0x3c)+24;$imageEnd=0x400000+[BitConverter]::ToUInt32($image,$optional+56)
function Raw([int]$address,[int]$count){$offset=if($address-lt0x489000){$address-0x400c00}else{$address-0x489000+555520};return ,([byte[]]$image[$offset..($offset+$count-1)])}
$header=[Collections.Generic.List[string]]::new();$fixes=[Collections.Generic.List[string]]::new();$constants=@{}
$header.Add('static void install_original_construct(unsigned char *p){unsigned long address,old;long displacement;int i;')
function Emit([byte[]]$bytes,[int]$at){$header.Add('{unsigned char bytes[]={'+($bytes-join',')+'};memcpy(p+'+$at+',bytes,sizeof(bytes));}')}
$ranges=@(@(0x43a920,0x43b2ab,0,'META/m4/target656/construct/original/0043a920.disasm.json'),@(0x43b2d0,0x43bc83,4096,'META/m4/target656/construct/original/0043b2d0.disasm.json'),@(0x47de20,0x47de47,8192,'META/m4/playable/ai-controls/ftol-asm.json'))
foreach($range in $ranges){
 $start,$end,$destination,$file=$range;Emit (Raw $start ($end-$start)) $destination;$cursor=$start
 foreach($row in(Get-Content "$root/$file" -Raw|ConvertFrom-Json)){
  $at=[Convert]::ToInt32($row.address,16);if($at-lt$start-or$at-ge$end){continue};$bytes=[Convert]::FromHexString($row.bytes)
  if($cursor-ne$at-or[Convert]::ToHexString((Raw $at $bytes.Length))-ne$row.bytes.ToUpper()){throw 'Original gap/mismatch'};$cursor+=$bytes.Length;$pos=$destination+$at-$start
  if($row.mnemonic-eq'CALL'-and$row.operands[0]-match'^0x'){
   $callee=[Convert]::ToInt32($row.operands[0].Substring(2),16)
   $target=switch($callee){0x47de20{'(long)(p+8192)'}0x476f20{'(long)oracle_allocate'}0x476e90{'(long)oracle_set_query'}0x40a8c0{'(long)oracle_locate'}0x47de50{'(long)oracle_unavailable'}0x47e190{'(long)oracle_unavailable'}default{throw 'Unreviewed constructor call'}}
   $fixes.Add('displacement='+$target+'-(long)(p+'+($pos+5)+');memcpy(p+'+($pos+1)+',&displacement,4);')
  }
  foreach($operand in $row.operands){foreach($match in [regex]::Matches($operand,'0x([0-9a-fA-F]+)')){
   $address=[Convert]::ToInt32($match.Groups[1].Value,16)
   if(($row.mnemonic-eq'CALL'-or$row.mnemonic-match'^J')-and$operand-notmatch'\['){continue}
   $target=$null
   if($address-eq0x489074){$target='(unsigned long)(p+20520)'}
   elseif($address-ge0x489000-and$address-lt0x48b4e8){if(!$constants.ContainsKey($address)){$constants[$address]=16384+16*$constants.Count;Emit (Raw $address 8) $constants[$address]};$target='(unsigned long)(p+'+$constants[$address]+')'}
   elseif($address-ge0x495e30-and$address-le0x495e38){$target='(unsigned long)(p+'+(20480+$address-0x495e30)+')'}
   elseif($address-ge0x4d1920-and$address-lt0x4d1984){$target='(unsigned long)(p+'+(24576+$address-0x4d1920)+')'}
   elseif($address-ge0x4d1a00-and$address-lt0x4d1a64){$target='(unsigned long)(p+'+(24704+$address-0x4d1a00)+')'}
   elseif($address-eq0x496578){$target='(unsigned long)(p+24832)'}elseif($address-eq0x4d047c){$target='(unsigned long)(p+24848)'}
   elseif($address-eq0x4dfac4){$target='(unsigned long)(p+20500)'}elseif($address-eq0x4dfab8){$target='(unsigned long)(p+20504)'}
   elseif($address-eq0x4dfb28){$target='(unsigned long)(p+20508)'}elseif($address-eq0x4dfb2c){$target='(unsigned long)(p+20512)'}elseif($address-eq0x4dfb30){$target='(unsigned long)(p+20516)'}
   elseif($address-eq0x43b2ac){$target='(unsigned long)(p+12288)'}elseif($address-eq0x43bc84){$target='(unsigned long)(p+12320)'}
   elseif($address-eq0x435e80){$target='(unsigned long)bh_actor_damage_query'}elseif($address-eq0x43a1d0){$target='(unsigned long)bh_actor_response_query'}elseif($address-eq0x4398c0){$target='(unsigned long)bh_actor_vehicle_tick'}
   elseif($address-eq0x435790){$target='(unsigned long)oracle_route_step'}elseif($address-eq0x434920){$target='(unsigned long)oracle_draw_step'}
   elseif($address-eq0x4905b4-or$address-eq0x492e20){$target='(unsigned long)(p+20600)'}
   elseif($address-ge0x400000-and$address-lt$imageEnd){throw ('Unreviewed constructor absolute operand {0:x}'-f$address)}
   if($target){$offset=-1;for($j=0;$j-le$bytes.Length-4;++$j){if([BitConverter]::ToInt32($bytes,$j)-eq$address){$offset=$j;break}};if($offset-lt0){throw 'Missing relocation'};$fixes.Add('address='+$target+';memcpy(p+'+($pos+$offset)+',&address,4);')}
  }}
 }
 if($cursor-ne$end){throw 'Incomplete constructor body'}
}
foreach($table in @(@(0x43b2ac,12288,0x43a920,0x43b2ab,0),@(0x43bc84,12320,0x43b2d0,0x43bc83,4096))){
 $address,$destination,$owner,$end,$base=$table;$bytes=Raw $address 24
 for($i=0;$i-lt6;++$i){$target=[BitConverter]::ToInt32($bytes,4*$i);if($target-lt$owner-or$target-ge$end){throw 'Foreign constructor switch destination'}}
 Emit $bytes $destination;$fixes.Add('for(i=0;i<6;++i){memcpy(&old,p+'+$destination+'+i*4,4);address=(unsigned long)p+'+$base+'+old-'+$owner+';memcpy(p+'+$destination+'+i*4,&address,4);}')
}
$header.AddRange($fixes);$header.Add('address=(unsigned long)oracle_unavailable;memcpy(p+20520,&address,4);memset(p+20480,0,12);}')
[IO.File]::WriteAllLines("$out/original-construct.h",$header)
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC';$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH";$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($name in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($name,$null,'Process')}
$build=Get-Content "$root/build/game/build.json" -Raw|ConvertFrom-Json
if(!$build.passed-or$build.debugging){throw 'Current release game objects required'}
foreach($source in $build.sources.PSObject.Properties){
 if((Get-FileHash (Join-Path $root $source.Name)).Hash.ToLower()-ne$source.Value){throw "Rebuild changed production source first: $($source.Name)"}
}
& "$root/analysis/toolchain-sp3/bin/CL.EXE" /nologo /c /ML /O2 /G5 /Oi /Gy /ID:/Decomp/dx7sdk/include /DDIRECT3D_VERSION=0x0500 /DDIRECTDRAW_VERSION=0x0500 "/Fo$out/ai_construct_original.obj" "$root/tests/ai_construct_original.c";if($LASTEXITCODE){throw 'Compile failed'}
$rsp=@($build.commands[-1].argv|Select-Object -Skip 1|Where-Object{$_-notmatch'bh_game_main.obj$|/out:|/map:|/subsystem:'}|ForEach-Object{'"'+$_+'"'})
$rsp+='/subsystem:console';$rsp+=('"'+$out+'/ai_construct_original.obj"');$rsp+=('"/out:'+$out+'/ai-construct-match.exe"');$rsp+=('"/map:'+$out+'/ai-construct-match.map"');[IO.File]::WriteAllLines("$out/link.rsp",$rsp)
& "$vc/bin/LINK.EXE" "@$out/link.rsp";if($LASTEXITCODE){throw 'Link failed'}
& "$out/ai-construct-match.exe"|Tee-Object -FilePath "$out/result.log";exit $LASTEXITCODE
