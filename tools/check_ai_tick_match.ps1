$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$out = Join-Path $root 'build/ai-byte-match'
New-Item -ItemType Directory -Force $out | Out-Null
$reference = Join-Path $root 'GAME_DATA/BHUNTER.EXE'
if ((Get-FileHash $reference).Hash -ne 'C86205E299119A66E27F687679EE7E978F26FA9A6C0BFB21F664AA6E74E848B1') { throw 'Reference differs' }
$bytes = [IO.File]::ReadAllBytes($reference)
$body = [byte[]]$bytes[(0x4398c0-0x400c00)..(0x439910-0x400c00-1)]
if ($body.Length -ne 80 -or $body[6] -ne 0xe8 -or $body[56] -ne 0xe8 -or
    0x4398c0+11+[BitConverter]::ToInt32($body,7) -ne 0x437ae0 -or
    0x4398c0+61+[BitConverter]::ToInt32($body,57) -ne 0x438ff0 -or
    [BitConverter]::ToInt32($body,19) -ne 0x489208) { throw 'Original boundary/dependencies differ' }
$constant = [byte[]]$bytes[(0x489208-0x489000+555520)..(0x489208-0x489000+555520+7)]
$boundsBody = [byte[]]$bytes[(0x4340f0-0x400c00)..(0x434199-0x400c00-1)]
$header = 'static void install_original_tick(unsigned char *p){long displacement;unsigned long address;unsigned char bytes[]={'+($body -join ',')+'};unsigned char constant[]={'+($constant -join ',')+'};memcpy(p,bytes,80);memcpy(p+256,constant,8);address=(unsigned long)(p+256);memcpy(p+19,&address,4);displacement=(long)bh_actor_attack_bound-(long)(p+11);memcpy(p+7,&displacement,4);displacement=(long)bh_actor_pursuit_bound-(long)(p+61);memcpy(p+57,&displacement,4);}'
[IO.File]::WriteAllText("$out/original-tick.h", $header)
Add-Content -LiteralPath "$out/original-tick.h" -Value ('static void install_original_bounds(unsigned char *p){unsigned char bytes[]={'+($boundsBody -join ',')+'};memcpy(p,bytes,sizeof(bytes));}')
$normalBody = [byte[]]$bytes[(0x434040-0x400c00)..(0x4340f0-0x400c00-1)]
if ([BitConverter]::ToInt32($normalBody,29) -ne 0x4340d4) { throw 'Normal jump-table operand differs' }
for ($i=0; $i -lt 7; ++$i) {
    $destination = [BitConverter]::ToInt32($normalBody,148+4*$i)
    if ($destination -lt 0x434040 -or $destination -ge 0x4340d3) { throw 'Normal jump-table target outside owner' }
}
Add-Content -LiteralPath "$out/original-tick.h" -Value ('static void install_original_normal(unsigned char *p){unsigned long address,old;int i;unsigned char bytes[]={'+($normalBody -join ',')+'};memcpy(p,bytes,sizeof(bytes));address=(unsigned long)(p+148);memcpy(p+29,&address,4);for(i=0;i<7;++i){memcpy(&old,p+148+4*i,4);address=(unsigned long)p+old-0x434040;memcpy(p+148+4*i,&address,4);}}')
$vc = 'D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC'
$sp = 'D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH = "$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH"
$env:INCLUDE = "$sp/include;$vc/include"
$env:LIB = "$sp/lib;$vc/lib"
foreach ($name in @('CL','_CL_','LINK','_LINK_')) { [Environment]::SetEnvironmentVariable($name,$null,'Process') }
$names = @('bh_actor_attack','bh_actor_response','bh_actor_pursuit_route','bh_camera_curve','bh_actor_route','bh_target_steering','bh_route_curve_length','bh_entities','bh_route_choice','bh_route_query')
$objects = @()
foreach ($name in ($names + @('ai_tick_match'))) {
    $folder = if ($name -eq 'ai_tick_match') { 'tests' } else { 'src' }
    & "$root/analysis/toolchain-sp3/bin/CL.EXE" /nologo /c /ML /O2 /G5 /Oi /Gy /ID:/Decomp/dx7sdk/include /DDIRECT3D_VERSION=0x0500 /DDIRECTDRAW_VERSION=0x0500 "/Fo$out/$name.obj" "$root/$folder/$name.c"
    if ($LASTEXITCODE) { throw "Compile failed: $name" }
    $objects += '"'+$out+'/'+$name+'.obj"'
}
& python "$root/tools/compare_ai_object.py" "$out/bh_actor_attack.obj" _bh_actor_vehicle_step 0x4398c0 80 --bind _bh_actor_attack_bound=0x437ae0 --bind _bh_actor_pursuit_bound=0x438ff0 --output "$out/production-object-comparison.json"
if ($LASTEXITCODE -or -not (Get-Content "$out/production-object-comparison.json" -Raw | ConvertFrom-Json).matched) { throw 'Whole-owner byte match failed' }
& python "$root/tools/compare_ai_object.py" "$out/bh_target_steering.obj" _bh_actor_bounds_clamp 0x4340f0 169 --output "$out/bounds-production-comparison.json"
if ($LASTEXITCODE -or -not (Get-Content "$out/bounds-production-comparison.json" -Raw | ConvertFrom-Json).matched) { throw 'Bounds byte match failed' }
& python "$root/tools/compare_ai_object.py" "$out/bh_route_choice.obj" _bh_actor_choose_exit_core 0x4344f0 344 --data-bind _bh_actor_exit_counts=0x497230 --output "$out/choice-production-comparison.json"
if ($LASTEXITCODE -or -not (Get-Content "$out/choice-production-comparison.json" -Raw | ConvertFrom-Json).matched) { throw 'Exit selector byte match failed' }
& python "$root/tools/compare_ai_object.py" "$out/bh_route_query.obj" _bh_actor_portal_normal_core 0x434040 147 --switch-table 0x4340d4=7 --output "$out/normal-production-comparison.json"
if ($LASTEXITCODE -or -not (Get-Content "$out/normal-production-comparison.json" -Raw | ConvertFrom-Json).matched) { throw 'Portal normal/table byte match failed' }
$exclude = 'runtime_fixes.obj|"/out:|"/map:|' + (($names | ForEach-Object { $_+'\.obj' }) -join '|')
$rsp = @(Get-Content "$root/build/runtime-fixes/link.rsp" | Where-Object { $_ -notmatch $exclude })
$rsp += $objects
$rsp += '"/out:'+$out+'/ai-tick-match.exe"'
$rsp += '"/map:'+$out+'/ai-tick-match.map"'
[IO.File]::WriteAllLines("$out/tick-link.rsp", $rsp)
& "$vc/bin/LINK.EXE" "@$out/tick-link.rsp"
if ($LASTEXITCODE) { throw 'Link failed' }
Push-Location $root
try { & "$out/ai-tick-match.exe" | Tee-Object -FilePath "$out/tick-result.log"; $result = $LASTEXITCODE } finally { Pop-Location }
exit $result
