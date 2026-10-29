# Build a private source snapshot so concurrent AI builds keep their own output.
param([string]$OutputDirectory='build/physics-source')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out=[IO.Path]::GetFullPath((Join-Path $root $OutputDirectory))
New-Item -ItemType Directory -Force "$out/src","$out/tests","$out/objects"|Out-Null
$sources=[ordered]@{}
Get-ChildItem "$root/src" -File | Where-Object {$_.Extension -in '.c','.h','.inc'} | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination "$out/src/$($_.Name)"
    $sources["src/$($_.Name)"]=(Get-FileHash "$out/src/$($_.Name)").Hash.ToLower()
}
$context=Get-Content "$root/tools/game_compile_context.json" -Raw|ConvertFrom-Json
$names=@($context.sources.PSObject.Properties.Name)+@('bh_game_app.c','bh_game_main.c')|Sort-Object -Unique
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC'
$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$sdk='D:/Decomp/dx7sdk'
$compiler="$root/analysis/toolchain-sp3/bin/CL.EXE"
$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH"
$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($name in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($name,$null,'Process')}
$commands=[Collections.Generic.List[object]]::new()
function RunTool([string]$tool,[string[]]$arguments){
    $result=& $tool @arguments 2>&1
    $code=$LASTEXITCODE
    $commands.Add([ordered]@{tool=$tool;arguments=$arguments;exit_code=$code;output=($result -join "`n")})
    if($code){$result|Write-Output;throw "Tool failed: $tool"}
}
$common=@('/nologo','/c','/ML','/O2','/G5','/Oi','/Gy')
$objects=@()
foreach($name in $names){
    $flags=$context.sources.$name
    if($null-eq$flags){$flags=@('/I{DXSDK}/include','/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500','/DDIRECTINPUT_VERSION=0x0500')}
    $flags=@($flags|ForEach-Object{$_.Replace('{DXSDK}',$sdk)})
    $object="$out/objects/$([IO.Path]::GetFileNameWithoutExtension($name)).obj"
    RunTool $compiler ($common+$flags+@("/Fo$object","$out/src/$name"))
    $objects+=$object
}
$libraries=@("$sdk/lib/ddraw.lib","$sdk/lib/dinput.lib",'kernel32.lib','user32.lib','gdi32.lib','dsound.lib','winmm.lib','ole32.lib','advapi32.lib','comctl32.lib')
function LinkTarget([string]$target,[string[]]$inputs,[string]$subsystem){
    $arguments=@('/nologo','/opt:ref',"/subsystem:$subsystem","/out:$out/$target.exe","/map:$out/$target.map")+$inputs+$libraries
    $rsp="$out/$target-link.rsp"
    [IO.File]::WriteAllLines($rsp,@($arguments|ForEach-Object{'"'+$_+'"'}))
    RunTool "$vc/bin/LINK.EXE" @("@$rsp")
}
LinkTarget 'BHunter-physics-source' $objects 'windows'
# Existing native contracts exercise connected collision/response/effects owners.
$cases=@('geometry','geometry_radius','collision_resource','collision_box','collision_cache','collision_model','collision_query','segment_query','segment_owner','sweep','entity_collision','physics_response','physics_effects','physics_step')
Get-ChildItem "$root/tests" -Filter '*.h' -File | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination "$out/tests/$($_.Name)"
    $sources["tests/$($_.Name)"]=(Get-FileHash "$out/tests/$($_.Name)").Hash.ToLower()
}
$driver=@('#include "../src/bh_fp.h"','#include <stdio.h>')
foreach($case in $cases){
    if($case-in@('geometry','collision_cache','collision_resource')){$driver+="int __cdecl bh_${case}_checkpoint(const char *, unsigned long *);"}
    else{$driver+="int __cdecl bh_${case}_checkpoint(unsigned long *);"}
}
$driver+=@('int main(void) { BHMathEnvironment fp; unsigned long count; int failed=0;',
           'if (!bh_math_environment_enter(&fp)) return 2;')
$testObjects=@()
foreach($case in $cases){
    Copy-Item -LiteralPath "$root/tests/$case.c" -Destination "$out/tests/$case.c"
    $sources["tests/$case.c"]=(Get-FileHash "$out/tests/$case.c").Hash.ToLower()
    $object="$out/objects/test_$case.obj"
    RunTool $compiler ($common+@("/I$sdk/include",'/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500','/DBH_GEOMETRY_RADIUS_LINKED',"/Fo$object","$out/tests/$case.c"))
    $testObjects+=$object
    if($case-in@('geometry','collision_cache','collision_resource')){$driver+="failed |= bh_${case}_checkpoint(0, &count);"}
    else{$driver+="failed |= bh_${case}_checkpoint(&count);"}
}
$driver+=@('bh_math_environment_leave(&fp); return failed != 0; }')
[IO.File]::WriteAllLines("$out/tests/driver.c",$driver)
RunTool $compiler ($common+@("/Fo$out/objects/driver.obj","$out/tests/driver.c"))
$contractObjects=@($objects|Where-Object{[IO.Path]::GetFileName($_)-ne'bh_game_main.obj'})+$testObjects+@("$out/objects/driver.obj")
LinkTarget 'physics-source-checks' $contractObjects 'console'
Push-Location $out
try{
    $result=& "$out/physics-source-checks.exe" 2>&1
    $code=$LASTEXITCODE
}finally{Pop-Location}
[IO.File]::WriteAllLines("$out/result.log",[string[]]$result)
$result|Write-Output
$record=[ordered]@{passed=($code-eq0);exit_code=$code;translation_units=$names.Count;target="$out/BHunter-physics-source.exe";executable_sha256=(Get-FileHash "$out/BHunter-physics-source.exe").Hash.ToLower();sources=$sources;commands=$commands;game_executed=$false;original_code_required=$false;validation_limit='Existing source contracts; no independent original fidelity claim'}
[IO.File]::WriteAllText("$out/validation.json",($record|ConvertTo-Json -Depth 12))
if($code){throw 'Physics source contracts failed'}
Write-Output "Linked $($names.Count) production translation units; physics source contracts passed."
