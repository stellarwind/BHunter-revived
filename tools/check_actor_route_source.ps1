# Existing focused source contracts, linked with current production definitions.
# No renderer, level, input, audio interface or original instruction execution.
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent;$out=Join-Path $root 'build/actor-route-source'
New-Item -ItemType Directory -Force $out|Out-Null
$build=Get-Content "$root/build/game/build.json" -Raw|ConvertFrom-Json
if(!$build.passed-or$build.debugging){throw 'Current release build required'}
foreach($source in @('src/bh_actor_route.c','src/bh_actor_route_promote_core.inc','src/bh_actor_route_lifetime.c','src/bh_actor_route_advance_core.inc','src/bh_actor_route_release_core.inc','src/bh_actor_route_due_core.inc','src/bh_actor_vehicle_lifetime.c','src/bh_actor_vehicle_release_core.inc','src/bh_route_guided.c','src/bh_route_query.c','src/bh_camera_curve.c','src/bh_entities.c','src/bh_sound_queue.c')){
 if((Get-FileHash "$root/$source").Hash.ToLower()-ne$build.sources.$source){throw 'Rebuild changed source first'}
}
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC';$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH";$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($name in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($name,$null,'Process')}
foreach($case in @(@('actor_route','BH_ACTOR_ROUTE_STANDALONE'),@('actor_route_lifetime','BH_ACTOR_ROUTE_LIFETIME_STANDALONE'),@('actor_vehicle_lifetime','BH_ACTOR_VEHICLE_LIFETIME_STANDALONE'))){
 $name,$define=$case
 & "$root/analysis/toolchain-sp3/bin/CL.EXE" /nologo /c /ML /O2 /G5 /Oi /Gy /ID:/Decomp/dx7sdk/include /DDIRECT3D_VERSION=0x0500 /DDIRECTDRAW_VERSION=0x0500 "/D$define" "/Fo$out/$name.obj" "$root/tests/$name.c"
 if($LASTEXITCODE){throw 'Source contract compile failed'}
 $rsp=@($build.commands[-1].argv|Select-Object -Skip 1|Where-Object{$_-notmatch'bh_game_main.obj$|/out:|/map:|/subsystem:'}|ForEach-Object{'"'+$_+'"'})
 $rsp+='/subsystem:console';$rsp+=('"'+$out+'/'+$name+'.obj"');$rsp+=('"/out:'+$out+'/'+$name+'.exe"');$rsp+=('"/map:'+$out+'/'+$name+'.map"');[IO.File]::WriteAllLines("$out/$name-link.rsp",$rsp)
 & "$vc/bin/LINK.EXE" "@$out/$name-link.rsp";if($LASTEXITCODE){throw 'Source contract link failed'}
 & "$out/$name.exe"|Tee-Object "$out/$name.log";if($LASTEXITCODE){throw 'Source contract failed'}
}
