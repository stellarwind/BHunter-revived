$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$out = Join-Path $root 'build/ai-motion'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$exeBytes = [IO.File]::ReadAllBytes((Join-Path $root 'GAME_DATA/BHUNTER.EXE'))
if ((Get-FileHash (Join-Path $root 'GAME_DATA/BHUNTER.EXE')).Hash -ne 'C86205E299119A66E27F687679EE7E978F26FA9A6C0BFB21F664AA6E74E848B1') { throw 'Reference hash differs' }
function Read-Reference([int]$address,[int]$length) {
    $offset = if ($address -lt 0x489000) {$address-0x401000+1024} else {$address-0x489000+555520}
    return ,([byte[]]$exeBytes[$offset..($offset+$length-1)])
}
$header = [Collections.Generic.List[string]]::new()
$fixes = [Collections.Generic.List[string]]::new()
$constants = @{}
$ranges = @(@(0x43a1d0,0x43a1df,0),@(0x43a3e7,0x43a57a,15),@(0x4341a0,0x4343bf,4096))
$motion = Get-Content (Join-Path $root 'META/m4/playable/ai-aggression/0043a1d0-asm.json') -Raw | ConvertFrom-Json
$curve = Get-Content (Join-Path $root 'META/m4/playable/ai-motion/curve-original-asm.json') -Raw | ConvertFrom-Json
$header.Add('static void install_original(unsigned char *p) { unsigned long address; long displacement;')
foreach ($range in $ranges) {
    $start,$end,$destination = $range
    $body = Read-Reference $start ($end-$start)
    $header.Add('{ unsigned char bytes[]={' + ($body -join ',') + '}; memcpy(p+'+$destination+',bytes,sizeof(bytes)); }')
    $rows = if ($start -eq 0x4341a0) {$curve} else {$motion}
    $cursor = $start
    foreach ($row in $rows) {
        $at = [Convert]::ToInt32($row.address,16)
        if ($at -lt $start -or $at -ge $end) {continue}
        if ($at -ne $cursor) {throw 'Disassembly gap'}
        $bytes = [Convert]::FromHexString($row.bytes)
        if ([Convert]::ToHexString((Read-Reference $at $bytes.Length)) -ne $row.bytes.ToUpper()) {throw 'Disassembly mismatch'}
        $cursor += $bytes.Length
        if ($row.mnemonic -eq 'CALL') {
            if ($row.operands[0] -ne '0x004341a0') {throw 'Unexpected call in pure closure'}
            $pos = $destination+$at-$start
            $fixes.Add('displacement=4096-('+($pos+5)+'); memcpy(p+'+($pos+1)+',&displacement,4);')
        }
        foreach ($operand in $row.operands) {
            if ($operand -match '\[0x([0-9a-fA-F]+)\]') {
                $address = [Convert]::ToInt32($Matches[1],16)
                if ($address -lt 0x489000 -or $address -ge 0x48b4e8) {throw 'Nonconstant global in pure closure'}
                $size = if ($operand.StartsWith('double')) {8} else {4}
                if (-not $constants.ContainsKey($address)) {
                    $slot=8192+16*$constants.Count; $constants[$address]=$slot
                    $data=Read-Reference $address 8
                    $header.Add('{ unsigned char bytes[]={'+($data -join ',')+'}; memcpy(p+'+$slot+',bytes,8); }')
                }
                $needle=[BitConverter]::GetBytes($address); $index=-1
                for($j=0;$j -le $bytes.Length-4;$j++){if([BitConverter]::ToInt32($bytes,$j) -eq $address){$index=$j;break}}
                if($index -lt 0){throw 'Missing constant operand'}
                $fixes.Add('address=(unsigned long)(p+'+$constants[$address]+'); memcpy(p+'+($destination+$at-$start+$index)+',&address,4);')
            }
        }
    }
    if($cursor -ne $end){throw 'Incomplete pure closure'}
}
$header.AddRange($fixes);$header.Add('}')
[IO.File]::WriteAllLines((Join-Path $out 'original.h'),$header)
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC'
$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH"
$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($name in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($name,$null,'Process')}
$compiler="$root/analysis/toolchain-sp3/bin/CL.EXE"
foreach($name in @('src/bh_actor_response.c','src/bh_entities.c','src/bh_camera_curve.c','src/bh_actor_route.c','tests/actor_motion_original.c')) {
    $stem=[IO.Path]::GetFileNameWithoutExtension($name)
    & $compiler /nologo /c /ML /O2 /G5 /Oi /Gy /ID:/Decomp/dx7sdk/include /DDIRECT3D_VERSION=0x0500 /DDIRECTDRAW_VERSION=0x0500 "/Fo$out/$stem.obj" "$root/$name"
    if($LASTEXITCODE){throw 'Compilation failed'}
}
$rsp = @(Get-Content "$root/build/runtime-fixes/link.rsp" | Where-Object {$_ -notmatch 'runtime_fixes.obj|bh_actor_response.obj|bh_entities.obj|bh_camera_curve.obj|bh_actor_route.obj|"/out:|"/map:'})
$rsp += @('"/out:'+$out+'/actor-motion.exe"','"'+$out+'/bh_actor_response.obj"','"'+$out+'/actor_motion_original.obj"')
$rsp += '"'+$out+'/bh_entities.obj"'
$rsp += '"'+$out+'/bh_camera_curve.obj"'
$rsp += '"'+$out+'/bh_actor_route.obj"'
[IO.File]::WriteAllLines("$out/link.rsp",$rsp)
& "$vc/bin/LINK.EXE" "@$out/link.rsp"
if($LASTEXITCODE){throw 'Link failed'}
Push-Location $out
try { & './actor-motion.exe' | Tee-Object -FilePath result.log; $result=$LASTEXITCODE } finally {Pop-Location}
exit $result
