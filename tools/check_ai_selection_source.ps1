# Focused existing target-selection checks linked with current production owners.
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent;$out=Join-Path $root 'build/ai-selection-match'
New-Item -ItemType Directory -Force $out|Out-Null
$build=Get-Content "$root/build/game/build.json" -Raw|ConvertFrom-Json
if(!$build.passed-or$build.debugging){throw 'Current release build required'}
foreach($p in @('src/bh_target_selection.c','src/bh_entities.c','src/bh_fp.c')){
 if((Get-FileHash "$root/$p").Hash.ToLower()-ne$build.sources.$p){throw 'Rebuild changed production source first'}
}
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC';$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH";$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($n in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($n,$null,'Process')}
& "$root/analysis/toolchain-sp3/bin/CL.EXE" /nologo /c /ML /O2 /G5 /Oi /Gy /ID:/Decomp/dx7sdk/include /DDIRECT3D_VERSION=0x0500 /DDIRECTDRAW_VERSION=0x0500 /DBH_TARGET_SELECTION_STANDALONE "/Fo$out/target_selection.obj" "$root/tests/target_selection.c"
if($LASTEXITCODE){throw 'Compile failed'}
$rsp=@($build.commands[-1].argv|Select-Object -Skip 1|Where-Object{$_-notmatch'bh_game_main.obj$|/out:|/map:|/subsystem:'}|ForEach-Object{'"'+$_+'"'})
$rsp+='/subsystem:console';$rsp+=('"'+$out+'/target_selection.obj"');$rsp+=('"/out:'+$out+'/target-selection-source.exe"');$rsp+=('"/map:'+$out+'/target-selection-source.map"');[IO.File]::WriteAllLines("$out/source-link.rsp",$rsp)
& "$vc/bin/LINK.EXE" "@$out/source-link.rsp";if($LASTEXITCODE){throw 'Link failed'}
Push-Location $out
try{& "$out/target-selection-source.exe"|Tee-Object "$out/source.log";$result=$LASTEXITCODE}finally{Pop-Location}
exit $result
