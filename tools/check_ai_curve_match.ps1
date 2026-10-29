param([string]$CandidateObject, [string]$CandidateSymbol)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out=Join-Path $root 'build/ai-curve-match'
New-Item -ItemType Directory -Force $out|Out-Null
$reference=Join-Path $root 'GAME_DATA/BHUNTER.EXE'
if((Get-FileHash $reference).Hash-ne'C86205E299119A66E27F687679EE7E978F26FA9A6C0BFB21F664AA6E74E848B1'){throw 'Reference differs'}
$bytes=[IO.File]::ReadAllBytes($reference)
function Raw([int]$a,[int]$n){$offset=if($a-lt0x489000){$a-0x400c00}else{$a-0x489000+555520};return ,([byte[]]$bytes[$offset..($offset+$n-1)])}
$body=Raw 0x4341a0 543
$header=@('static void install_original_curve(unsigned char *p){unsigned long address;',
 ('{unsigned char bytes[]={'+($body-join',')+'};memcpy(p,bytes,sizeof(bytes));}'))
$cursor=0x4341a0;$constantOffset=1024;$relocations=0
foreach($row in(Get-Content "$root/META/m4/playable/ai-motion/curve-original-asm.json" -Raw|ConvertFrom-Json)){
 $at=[Convert]::ToInt32($row.address,16);if($at-lt0x4341a0-or$at-ge0x4343bf){continue}
 $data=[Convert]::FromHexString($row.bytes)
 if($at-ne$cursor-or[Convert]::ToHexString((Raw $at $data.Length))-ne$row.bytes.ToUpper()){throw 'Original instruction mismatch/gap'}
 $cursor+=$data.Length
 if($row.mnemonic-eq'CALL'){throw 'Unexpected original callee'}
 foreach($operand in $row.operands){
  if($operand-match'ptr \[0x(00489[0-9a-f]+)\]'){
   $va=[Convert]::ToInt32($Matches[1],16);$size=if($operand.StartsWith('double')){8}else{4}
   if($data.Length-ne6-or[BitConverter]::ToInt32($data,2)-ne$va){throw 'Unexpected constant encoding'}
   $constant=Raw $va $size
   $header+=('{unsigned char bytes[]={'+($constant-join',')+'};memcpy(p+'+$constantOffset+',bytes,sizeof(bytes));}')
   $header+=('address=(unsigned long)(p+'+$constantOffset+');memcpy(p+'+($at-0x4341a0+2)+',&address,4);')
   $constantOffset+=8;++$relocations
  }elseif($row.mnemonic-match'^J'-and$operand-match'^0x'){
   $target=[Convert]::ToInt32($operand.Substring(2),16)
   if($target-lt0x4341a0-or$target-ge0x4343bf){throw 'External original branch'}
  }
 }
}
if($cursor-ne0x4343bf-or$relocations-ne6){throw 'Incomplete curve owner/constant closure'}
$header+='}';[IO.File]::WriteAllLines("$out/original-curve.h",$header)
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC';$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH";$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($name in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($name,$null,'Process')}
$build=Get-Content "$root/build/game/build.json" -Raw|ConvertFrom-Json
$cache=Get-Content "$root/build/game/objects.json" -Raw|ConvertFrom-Json
$objects=@()
foreach($name in @('bh_camera_curve','bh_round','bh_fp')){
 if(!$build.passed-or$build.debugging-or(Get-FileHash "$root/src/$name.c").Hash.ToLower()-ne$build.sources."src/$name.c"){throw 'Current production objects required'}
 if((Get-FileHash "$root/build/game/$name.obj").Hash.ToLower()-ne$cache."$name.c".object_sha256){throw 'Production object hash differs'}
 $objects+=('"'+$root+'/build/game/'+$name+'.obj"')
}
$flags=@();if($CandidateObject){
 if(!$CandidateSymbol-or$CandidateSymbol-notmatch'^[a-zA-Z_][a-zA-Z_0-9]*$'){throw 'Candidate symbol required'}
 $flags+=('/DBH_CURVE_CANDIDATE='+$CandidateSymbol);$objects+=('"'+(Resolve-Path $CandidateObject).Path+'"')
}
& "$root/analysis/toolchain-sp3/bin/CL.EXE" /nologo /c /ML /O2 /G5 /Oi /Gy @flags "/Fo$out/ai_curve_original.obj" "$root/tests/ai_curve_original.c"
if($LASTEXITCODE){throw 'Compile failed'}
$rsp=@('/nologo','kernel32.lib')+$objects+@('"'+$out+'/ai_curve_original.obj"','"/out:'+$out+'/ai-curve-match.exe"','"/map:'+$out+'/ai-curve-match.map"')
[IO.File]::WriteAllLines("$out/link.rsp",$rsp)
& "$vc/bin/LINK.EXE" "@$out/link.rsp";if($LASTEXITCODE){throw 'Link failed'}
& "$out/ai-curve-match.exe"|Tee-Object "$out/result.log";exit $LASTEXITCODE
