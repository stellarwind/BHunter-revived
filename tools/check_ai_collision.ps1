$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out=Join-Path $root 'build/ai-collision'
New-Item -ItemType Directory -Force $out|Out-Null
$exeBytes=[IO.File]::ReadAllBytes("$root/GAME_DATA/BHUNTER.EXE")
if((Get-FileHash "$root/GAME_DATA/BHUNTER.EXE").Hash -ne 'C86205E299119A66E27F687679EE7E978F26FA9A6C0BFB21F664AA6E74E848B1'){throw 'Reference hash differs'}
function Raw([int]$a,[int]$n){$o=if($a-lt0x489000){$a-0x401000+1024}else{$a-0x489000+555520};return ,([byte[]]$exeBytes[$o..($o+$n-1)])}
$body=Raw 0x44d530 (0x44d9ab-0x44d530)
if([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($body)).ToLower()-ne'712684e04eabf2ba829bb0f197ec1845177fea1965b758cedc9c549451b42f1f'){throw 'Owner body differs'}
$header=[Collections.Generic.List[string]]::new()
$header.Add('static void install_original(unsigned char *p){unsigned long address;long displacement;')
$header.Add('{unsigned char bytes[]={'+($body-join',')+'};memcpy(p,bytes,sizeof(bytes));}')
$constants=Raw 0x489ea0 24
$header.Add('{unsigned char bytes[]={'+($constants-join',')+'};memcpy(p+2048,bytes,sizeof(bytes));}')
# Full recursive segment owner only. Explicit borrowed geometry/globals at4096;
# no loader, imports, callbacks, or original game startup. Math calls use the
# existing real source math definitions (separate dependency evidence).
$lines=@(Get-Content "$root/META/m4/segment-query/original/0044d530.asm.txt"|Where-Object{$_-match'^[0-9a-f]{8} '})
for($i=0;$i-lt$lines.Count;$i++){
 $line=$lines[$i];$at=[Convert]::ToInt32($line.Substring(0,8),16)
 $end=if($i+1-lt$lines.Count){[Convert]::ToInt32($lines[$i+1].Substring(0,8),16)}else{0x44d9ab}
 $bytes=Raw $at ($end-$at);$pos=$at-0x44d530
 if($line-match' CALL 0x([0-9a-f]+)'){
  $callee=[Convert]::ToInt32($Matches[1],16)
  if($bytes[0]-ne0xe8-or$at+5+[BitConverter]::ToInt32($bytes,1)-ne$callee){throw 'Call mismatch'}
  $target=switch($callee){0x44d530{'p'} 0x4011a0{'bh_vec3_cross'} 0x4012e0{'bh_mat34_transform_point'} 0x401360{'bh_mat34_transform_direction'} default{throw 'Unexpected call'}}
  $header.Add('displacement=(long)'+$target+'-(long)(p+'+($pos+5)+');memcpy(p+'+($pos+1)+',&displacement,4);')
 }
 if($line-match'\[0x([0-9a-f]+)\]'){
  $address=[Convert]::ToInt32($Matches[1],16)
  $slot=if($address-ge0x4e0fd0-and$address-le0x4e104c){4096+$address-0x4e0fd0}elseif($address-ge0x489ea0-and$address-le0x489eb0){2048+$address-0x489ea0}else{throw 'Unexpected global'}
  $index=-1;for($j=0;$j-le$bytes.Length-4;$j++){if([BitConverter]::ToInt32($bytes,$j)-eq$address){$index=$j;break}}
  if($index-lt0){throw 'Missing relocation'}
  $header.Add('address=(unsigned long)(p+'+$slot+');memcpy(p+'+($pos+$index)+',&address,4);')
 }
}
$header.Add('}');[IO.File]::WriteAllLines("$out/original.h",$header)
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC';$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH";$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($name in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($name,$null,'Process')}
$names=@('bh_segment_query','bh_bsp','bh_geometry');$objects=@()
foreach($name in ($names+@('ai_collision_original'))){
 $folder=if($name-eq'ai_collision_original'){'tests'}else{'src'}
 & "$root/analysis/toolchain-sp3/bin/CL.EXE" /nologo /c /ML /O2 /G5 /Oi /Gy "/Fo$out/$name.obj" "$root/$folder/$name.c"
 if($LASTEXITCODE){throw 'Compile failed'};$objects+='"'+$out+'/'+$name+'.obj"'
}
$exclude='runtime_fixes.obj|"/out:|"/map:|'+(($names|ForEach-Object{$_+'\.obj'})-join'|')
$rsp=@(Get-Content "$root/build/runtime-fixes/link.rsp"|Where-Object{$_-notmatch$exclude})
$rsp+=$objects;$rsp+='"/out:'+$out+'/ai-collision.exe"'
[IO.File]::WriteAllLines("$out/link.rsp",$rsp)
& "$vc/bin/LINK.EXE" "@$out/link.rsp";if($LASTEXITCODE){throw 'Link failed'}
Push-Location $root
try{& "$out/ai-collision.exe"|Tee-Object -FilePath "$out/result.log";$result=$LASTEXITCODE}finally{Pop-Location}
exit $result
