# Isolated route/smoke subpath with shared real typed dependencies. No game execution.
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent;$out=Join-Path $root 'build/ai-response-smoke-match'
New-Item -ItemType Directory -Force $out|Out-Null
$reference=Join-Path $root 'GAME_DATA/BHUNTER.EXE'
if((Get-FileHash $reference).Hash-ne'C86205E299119A66E27F687679EE7E978F26FA9A6C0BFB21F664AA6E74E848B1'){throw 'Reference differs'}
$image=[IO.File]::ReadAllBytes($reference)
function Raw([int]$a,[int]$n){$offset=if($a-lt0x489000){$a-0x400c00}else{$a-0x489000+555520};return ,([byte[]]$image[$offset..($offset+$n-1)])}
# cdecl wrapper preserves registers and supplies the original scratch frame.
# Explicit exits capture the saved death position and smoke/skip outcome.
$prefix=[Convert]::FromHexString('5355565781ecac0000008bb424c0000000')
$suffix=[Convert]::FromHexString('83c4348b8c24c40000008b44244089018b4424448941048b442448894108b801000000eb0233c081c4ac0000005f5e5d5bc3')
$header=[Collections.Generic.List[string]]::new();$fixes=[Collections.Generic.List[string]]::new();$constants=@{}
$header.Add('static void install_original_smoke(unsigned char *p){unsigned long address;long displacement;')
function Emit([byte[]]$bytes,[int]$at){$header.Add('{unsigned char bytes[]={'+($bytes-join',')+'};memcpy(p+'+$at+',bytes,sizeof(bytes));}')}
$start=0x439bfe;$end=0x439dcd;$skip=$prefix.Length+($end-$start)+37
if($suffix[37]-ne0x33){throw 'Skip adapter offset differs'}
Emit $prefix 0;Emit $suffix ($prefix.Length+$end-$start)
$ranges=@(@($start,$end,$prefix.Length,'META/m4/target656/response/original/00439910.disasm.json'),@(0x4341a0,0x4343bf,2048,'META/m4/playable/ai-motion/curve-original-asm.json'))
foreach($range in $ranges){
 $begin,$finish,$destination,$file=$range;Emit (Raw $begin ($finish-$begin)) $destination;$cursor=$begin
 foreach($row in(Get-Content "$root/$file" -Raw|ConvertFrom-Json)){
  $at=[Convert]::ToInt32($row.address,16);if($at-lt$begin-or$at-ge$finish){continue}
  $bytes=[Convert]::FromHexString($row.bytes)
  if($at-ne$cursor-or[Convert]::ToHexString((Raw $at $bytes.Length))-ne$row.bytes.ToUpper()){throw 'Original instruction gap/mismatch'};$cursor+=$bytes.Length
  $pos=$destination+$at-$begin
  if($row.mnemonic-eq'CALL'){
   $target=switch($row.operands[0]){'0x00438ff0'{'(long)oracle_pursuit'}'0x0047ddf0'{'(long)rand'}'0x004341a0'{'(long)(p+2048)'}'0x004025c0'{'(long)oracle_particle'}default{throw 'Unreviewed call'}}
   $fixes.Add('displacement='+$target+'-(long)(p+'+($pos+5)+');memcpy(p+'+($pos+1)+',&displacement,4);');continue
  }
  foreach($operand in $row.operands){foreach($match in [regex]::Matches($operand,'0x([0-9a-fA-F]+)')){
   $address=[Convert]::ToInt32($match.Groups[1].Value,16)
   if($row.mnemonic-match'^J'){
    if($address-eq0x43a18f){
     if($row.mnemonic-ne'JNZ'-or$bytes.Length-ne6){throw 'Unexpected skip branch'}
     $fixes.Add('displacement=(long)(p+'+$skip+')-(long)(p+'+($pos+6)+');memcpy(p+'+($pos+2)+',&displacement,4);')
    }elseif($address-lt$begin-or$address-ge$finish){throw 'Foreign branch'}
    continue
   }
   $target=$null
   if($address-ge0x489000-and$address-lt0x48b4e8){
    if(!$constants.ContainsKey($address)){$constants[$address]=4096+16*$constants.Count;Emit (Raw $address 8) $constants[$address]}
    $target='(unsigned long)(p+'+$constants[$address]+')'
   }elseif($address-eq0x4f0028){$target='(unsigned long)(p+3072)'}
   elseif($address-ge0x400000-and$address-lt0x600000){throw 'Unreviewed absolute operand'}
   if($target){
    $offset=-1;for($j=0;$j-le$bytes.Length-4;++$j){if([BitConverter]::ToInt32($bytes,$j)-eq$address){$offset=$j;break}}
    if($offset-lt0){throw 'Operand bytes absent'}
    $fixes.Add('address='+$target+';memcpy(p+'+($pos+$offset)+',&address,4);')
   }
  }}
 }
 if($cursor-ne$finish){throw 'Incomplete original window/curve'}
}
$header.AddRange($fixes);$header.Add('}');[IO.File]::WriteAllLines("$out/original-smoke.h",$header)
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC';$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH";$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($name in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($name,$null,'Process')}
$build=Get-Content "$root/build/game/build.json" -Raw|ConvertFrom-Json
if(!$build.passed-or$build.debugging){throw 'Current release objects required'}
foreach($source in @('src/bh_actor_response.c','src/bh_actor_response_motion.inc','src/bh_actor_response_smoke.inc','src/bh_actor_response_finish.inc','src/bh_actor_response_query_core.inc')){
 if((Get-FileHash "$root/$source").Hash.ToLower()-ne$build.sources.$source){throw 'Rebuild changed production source first'}
}
& "$root/analysis/toolchain-sp3/bin/CL.EXE" /nologo /c /ML /O2 /G5 /Oi /Gy /ID:/Decomp/dx7sdk/include /DDIRECT3D_VERSION=0x0500 /DDIRECTDRAW_VERSION=0x0500 "/Fo$out/ai_response_smoke_original.obj" "$root/tests/ai_response_smoke_original.c"
if($LASTEXITCODE){throw 'Compile failed'}
$rsp=@($build.commands[-1].argv|Select-Object -Skip 1|Where-Object{$_-notmatch'bh_game_main.obj$|/out:|/map:|/subsystem:'}|ForEach-Object{'"'+$_+'"'})
$rsp+='/subsystem:console';$rsp+=('"'+$out+'/ai_response_smoke_original.obj"');$rsp+=('"/out:'+$out+'/ai-response-smoke-match.exe"');$rsp+=('"/map:'+$out+'/ai-response-smoke-match.map"');[IO.File]::WriteAllLines("$out/link.rsp",$rsp)
& "$vc/bin/LINK.EXE" "@$out/link.rsp";if($LASTEXITCODE){throw 'Link failed'}
& "$out/ai-response-smoke-match.exe"|Tee-Object "$out/result.log";exit $LASTEXITCODE
