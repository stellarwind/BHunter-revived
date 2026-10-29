# Focused existing native source checks; no game, renderer or audio interfaces.
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent;$out=Join-Path $root 'build/actor-damage-source'
New-Item -ItemType Directory -Force $out|Out-Null
$build=Get-Content "$root/build/game/build.json" -Raw|ConvertFrom-Json
if(!$build.passed-or$build.debugging){throw 'Current release build required'}
foreach($p in @('src/bh_actor_damage.c','src/bh_actor_damage_query_core.inc','src/bh_actor_damage_tick_core.inc','src/bh_actor_response.c','src/bh_actor_response_query_core.inc','src/bh_actor_response_motion.inc','src/bh_actor_response_smoke.inc','src/bh_actor_response_finish.inc','src/bh_actor_route.c','src/bh_actor_route_promote_core.inc','src/bh_actor_route_lifetime.c','src/bh_entities.c','src/bh_sound_queue.c','src/bh_script_lifetime.c','src/bh_camera_curve.c')){
 if((Get-FileHash "$root/$p").Hash.ToLower()-ne$build.sources.$p){throw 'Rebuild changed production source first'}
}
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC';$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH";$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($n in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($n,$null,'Process')}
foreach($case in @(@('actor_damage','BH_ACTOR_DAMAGE_STANDALONE'),@('actor_response','BH_ACTOR_RESPONSE_STANDALONE'),@('script_lifetime','BH_SCRIPT_LIFETIME_STANDALONE'))){
 $name,$define=$case
 & "$root/analysis/toolchain-sp3/bin/CL.EXE" /nologo /c /ML /O2 /G5 /Oi /Gy /ID:/Decomp/dx7sdk/include /DDIRECT3D_VERSION=0x0500 /DDIRECTDRAW_VERSION=0x0500 "/D$define" "/Fo$out/$name.obj" "$root/tests/$name.c"
 if($LASTEXITCODE){throw 'Compile failed'}
 $rsp=@($build.commands[-1].argv|Select-Object -Skip 1|Where-Object{$_-notmatch'bh_game_main.obj$|/out:|/map:|/subsystem:'}|ForEach-Object{'"'+$_+'"'})
 $rsp+='/subsystem:console';$rsp+=('"'+$out+'/'+$name+'.obj"');$rsp+=('"/out:'+$out+'/'+$name+'.exe"');$rsp+=('"/map:'+$out+'/'+$name+'.map"');[IO.File]::WriteAllLines("$out/$name-link.rsp",$rsp)
 & "$vc/bin/LINK.EXE" "@$out/$name-link.rsp";if($LASTEXITCODE){throw 'Link failed'}
 Push-Location $out
 try{& "$out/$name.exe"|Tee-Object "$out/$name.log";$result=$LASTEXITCODE}finally{Pop-Location}
 if($result){throw 'Native source check failed'}
}
