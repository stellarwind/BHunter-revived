# Rebuild retained source variants in a separate oracle target. Shipping sources
# and immutable original references are never edited by this negative control.
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out="$root/build/collision-cut-baseline";$evidence="$root/META/m4/playable/collision-narrowphase"
New-Item -ItemType Directory -Force $out|Out-Null
$variants=@(
 @('bh_collision_model','META/m4/playable/collision-close/sources/aa0bcb6ccb74c9184f3ed3846adec7150c1ce3ef0ca0480d447f8abf2d45c253-bh_collision_model.c.txt','aa0bcb6ccb74c9184f3ed3846adec7150c1ce3ef0ca0480d447f8abf2d45c253'),
 @('bh_collision_query','META/m4/playable/collision-iteration/sources/0600bf0d7f2c04beaf9fddb8990deb6c3ca89b1d646ed7a664ec4dba00670ab7-bh_collision_query.c.txt','0600bf0d7f2c04beaf9fddb8990deb6c3ca89b1d646ed7a664ec4dba00670ab7')
)
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC';$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH";$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($name in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($name,$null,'Process')}
$rsp=Get-Content "$root/build/collision-full-original/link.rsp"
foreach($v in $variants){
 $name,$path,$hash=$v
 if((Get-FileHash "$root/$path").Hash.ToLower()-ne$hash){throw 'Retained baseline source differs'}
 Copy-Item -LiteralPath "$root/$path" -Destination "$out/$name.c"
 & "$root/analysis/toolchain-sp3/bin/CL.EXE" /nologo /c /ML /O2 /G5 /Oi /Gy "/I$root/src" "/Fo$out/$name.obj" "$out/$name.c"
 if($LASTEXITCODE){throw 'Real retained source TU compile failed'}
 $rsp=@($rsp|ForEach-Object{if($_.Trim('"')-eq"$root/build/collision-full-original/$name.obj"){'"'+"$out/$name.obj"+'"'}else{$_}})
}
$rsp=@($rsp|Where-Object{$_-notmatch'^"/out:|^"/map:'})+@('"'+"/out:$out/baseline.exe"+'"','"'+"/map:$out/baseline.map"+'"')
[IO.File]::WriteAllLines("$out/link.rsp",$rsp);& "$vc/bin/LINK.EXE" "@$out/link.rsp"
if($LASTEXITCODE){throw 'Real baseline link failed'}
$text=& "$out/baseline.exe";$status=$LASTEXITCODE;$text|Set-Content "$out/result.log"
$result=$text|Select-Object -Last 1|ConvertFrom-Json
if($status-eq0-or!$result.differences-or!$result.cached_differences){throw 'Expected original-backed baseline differences were not rejected'}
@{passed=$true;baseline_rejected=$true;target='build/collision-cut-baseline/baseline.exe';target_sha256=(Get-FileHash "$out/baseline.exe").Hash.ToLower();result=$result;source_variants=$variants;limit='Old complete source TUs under the current header context; source references retained separately from the corrected variant; no shipping code changed'}|ConvertTo-Json -Depth 8|Set-Content "$evidence/baseline-control.json"
$result|ConvertTo-Json -Compress
