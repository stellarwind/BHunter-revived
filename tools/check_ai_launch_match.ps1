param([switch]$Receive)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent;$out=Join-Path $root $(if($Receive){'build/ai-receive-match'}else{'build/ai-launch-match'})
New-Item -ItemType Directory -Force $out|Out-Null
$reference=Join-Path $root 'GAME_DATA/BHUNTER.EXE'
if((Get-FileHash $reference).Hash-ne'C86205E299119A66E27F687679EE7E978F26FA9A6C0BFB21F664AA6E74E848B1'){throw 'Reference differs'}
$bytes=[IO.File]::ReadAllBytes($reference)
$optional=[BitConverter]::ToInt32($bytes,0x3c)+24;$imageEnd=0x400000+[BitConverter]::ToUInt32($bytes,$optional+56)
function Raw([int]$a,[int]$n){$offset=if($a-lt0x489000){$a-0x400c00}else{$a-0x489000+555520};return ,([byte[]]$bytes[$offset..($offset+$n-1)])}
$header=[Collections.Generic.List[string]]::new();$fixes=[Collections.Generic.List[string]]::new();$constants=@{}
$header.Add('static void install_original_launch(unsigned char *p){unsigned long address;long displacement;')
function Emit([byte[]]$data,[int]$at){$header.Add('{unsigned char bytes[]={'+($data-join',')+'};memcpy(p+'+$at+',bytes,sizeof(bytes));}')}
$ranges=@(@(0x43bca0,0x43beb5,0,'META/m4/target656/launch/original/0043bca0.disasm.json'),@(0x4234d0,0x423655,1024,'META/m4/network-codec/original/004234d0.disassembly.json'),@(0x47de20,0x47de47,4096,'META/m4/playable/ai-controls/ftol-asm.json'))
if($Receive){$ranges=@(@(0x426d50,0x426ead,0,'META/m4/target656/launch/original/00426d50.disasm.json'),@(0x423840,0x42399d,1024,'META/m4/network-codec/original/00423840.disassembly.json'))}
foreach($range in $ranges){
 $start,$end,$destination,$file=$range;Emit (Raw $start ($end-$start)) $destination;$cursor=$start
 foreach($row in(Get-Content "$root/$file" -Raw|ConvertFrom-Json)){
  $at=[Convert]::ToInt32($row.address,16);if($at-lt$start-or$at-ge$end){continue};$data=[Convert]::FromHexString($row.bytes)
  if($cursor-ne$at-or[Convert]::ToHexString((Raw $at $data.Length))-ne$row.bytes.ToUpper()){throw 'Original instruction gap/mismatch'};$cursor+=$data.Length;$pos=$destination+$at-$start
  if($row.mnemonic-eq'CALL'){
   $target=switch([Convert]::ToInt32($row.operands[0].Substring(2),16)){0x47ddf0{'(long)oracle_rand'}0x47de20{'(long)(p+4096)'}0x4234d0{'(long)(p+1024)'}0x43a920{'(long)oracle_construct'}0x476d50{'(long)oracle_send'}0x423840{'(long)(p+1024)'}0x43b2d0{'(long)oracle_construct_position'}default{throw 'Unreviewed original launch call'}}
   $fixes.Add('displacement='+$target+'-(long)(p+'+($pos+5)+');memcpy(p+'+($pos+1)+',&displacement,4);')
  }
  foreach($operand in $row.operands){foreach($match in [regex]::Matches($operand,'0x([0-9a-fA-F]+)')){
   $address=[Convert]::ToInt32($match.Groups[1].Value,16)
   if($row.mnemonic-eq'CALL'-or($row.mnemonic-match'^J'-and$operand-notmatch'\[')-or$address-lt0x400000-or$address-ge$imageEnd){continue}
   if($Receive-and$address-eq0x496bd8){$slot=12288}elseif($Receive-and$address-eq0x4cdba0){$slot=12292}
   elseif($Receive-and$address-eq0x4dfacc){$slot=12296}elseif($Receive-and$address-eq0x4dfac8){$slot=12300}
   elseif($Receive-and$address-eq0x4dfad0){$slot=12304}elseif($Receive-and$address-eq0x4dfac4){$slot=12308}
   elseif($Receive-and$address-eq0x4df26c){$slot=12312}elseif($Receive-and$address-eq0x4df268){$slot=12316}
   elseif($address-eq0x4dfb08){$slot=12288}elseif($address-eq0x4dfac4){$slot=12292}elseif($address-eq0x4d1a74){$slot=12296}
   elseif($address-ge0x4d1490-and$address-le0x4d1498){$slot=16384+$address-0x4d1490}
   elseif($address-ge0x489000-and$address-lt0x48b4e8){if(!$constants.ContainsKey($address)){$constants[$address]=8192+16*$constants.Count;Emit (Raw $address 8) $constants[$address]};$slot=$constants[$address]}
   else{throw ('Unreviewed original absolute operand {0:x}'-f$address)}
   $offset=-1;for($j=0;$j-le$data.Length-4;++$j){if([BitConverter]::ToInt32($data,$j)-eq$address){$offset=$j;break}};if($offset-lt0){throw 'Missing original operand'}
   $fixes.Add('address=(unsigned long)(p+'+$slot+');memcpy(p+'+($pos+$offset)+',&address,4);')
  }}
 }
 if($cursor-ne$end){throw 'Incomplete original launch/position/CRT boundary'}
}
$header.AddRange($fixes);$header.Add('}');[IO.File]::WriteAllLines("$out/original-launch.h",$header)
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC';$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH";$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($name in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($name,$null,'Process')}
$build=Get-Content "$root/build/game/build.json" -Raw|ConvertFrom-Json
if(!$build.passed-or$build.debugging){throw 'Current release game objects required'}
foreach($source in @('src/bh_actor_launch.c','src/bh_actor_construct.c','src/bh_network_codec.c','src/bh_position.c','src/bh_network_transport.c')){if((Get-FileHash "$root/$source").Hash.ToLower()-ne$build.sources.$source){throw 'Rebuild changed production source first'}}
$test=if($Receive){'ai_receive_original'}else{'ai_launch_original'}
& "$root/analysis/toolchain-sp3/bin/CL.EXE" /nologo /c /ML /O2 /G5 /Oi /Gy /ID:/Decomp/dx7sdk/include /DDIRECT3D_VERSION=0x0500 /DDIRECTDRAW_VERSION=0x0500 "/Fo$out/$test.obj" "$root/tests/$test.c";if($LASTEXITCODE){throw 'Compile failed'}
$rsp=@($build.commands[-1].argv|Select-Object -Skip 1|Where-Object{$_-notmatch'bh_game_main.obj$|/out:|/map:|/subsystem:'}|ForEach-Object{'"'+$_+'"'})
$rsp+='/subsystem:console';$rsp+=('"'+$out+'/'+$test+'.obj"');$rsp+=('"/out:'+$out+'/ai-launch-match.exe"');$rsp+=('"/map:'+$out+'/ai-launch-match.map"');[IO.File]::WriteAllLines("$out/link.rsp",$rsp)
& "$vc/bin/LINK.EXE" "@$out/link.rsp";if($LASTEXITCODE){throw 'Link failed'}
& "$out/ai-launch-match.exe"|Tee-Object "$out/result.log";exit $LASTEXITCODE
