$ErrorActionPreference = 'Stop'
$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$tools = Join-Path $repo 'analysis\toolchain-sp3\bin'
$vc = 'D:\Decomp\MSCVC\MSVC5\TOOLS\DEVSTUDIO\VC'
$sp3 = 'D:\Decomp\MSCVC\MSVC5\SP3_EXTRACTED\VS97_SP3\all\vc'
$env:PATH = "$tools;" + $env:PATH
$env:PATH = 'D:\Decomp\MSCVC\MSVC5\TOOLS\DEVSTUDIO\SHAREDIDE\BIN;' + $env:PATH
$env:INCLUDE = "$sp3\include;$vc\include"
$env:LIB = "$sp3\lib;$vc\lib"
Push-Location $PSScriptRoot
try {
    & "$tools\CL.EXE" /nologo /c /ML /O2 /G5 /Oi /Gy /Fobh_math.obj "$repo\src\bh_math.c"
    if ($LASTEXITCODE -ne 0) { throw 'Math compilation failed' }
    & "$tools\CL.EXE" /nologo /c /ML /O2 /G5 /Oi /Fodifferential.obj differential.c
    if ($LASTEXITCODE -ne 0) { throw 'Harness compilation failed' }
    # Stock linker runs on this host; SP3 linker currently fails DLL symbol resolution.
    & "$vc\bin\LINK.EXE" /nologo /base:0x10000000 /fixed /subsystem:console /out:differential.exe differential.obj bh_math.obj kernel32.lib
    if ($LASTEXITCODE -ne 0) { throw 'Harness link failed' }
    & .\differential.exe "$repo\GAME_DATA\BHUNTER.EXE" | Tee-Object -FilePath differential-results.txt
    if ($LASTEXITCODE -ne 0) { throw 'Differential comparison failed; inspect results' }
} finally { Pop-Location }
