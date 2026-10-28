$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out=Join-Path $root 'build/ai-decisions'
New-Item -ItemType Directory -Force $out|Out-Null
if((Get-FileHash "$root/GAME_DATA/BHUNTER.EXE").Hash-ne'C86205E299119A66E27F687679EE7E978F26FA9A6C0BFB21F664AA6E74E848B1'){throw 'Reference differs'}
$exeBytes=[IO.File]::ReadAllBytes("$root/GAME_DATA/BHUNTER.EXE")
$rows=Import-Csv "$root/META/m4/target-steering/original/004344f0.tsv" -Delimiter "`t"
$bytes=[Collections.Generic.List[byte]]::new()
foreach($r in $rows){$at=[Convert]::ToInt32($r.address,16);$b=[Convert]::FromHexString($r.bytes);if([Convert]::ToHexString($exeBytes[($at-0x401000+1024)..($at-0x401000+1024+$b.Length-1)])-ne$r.bytes.ToUpper()){throw 'Instruction differs'};$bytes.AddRange($b)}
if($bytes.Count-ne344){throw 'Boundary differs'}
$h='static void install_original(unsigned char *p){unsigned long registry=(unsigned long)(p+1024);unsigned char bytes[]={'+($bytes-join',')+'};memcpy(p,bytes,sizeof(bytes));memcpy(p+48,&registry,4);}'
# 0043451d LEA registry operand begins at original+48.
if([BitConverter]::ToInt32($bytes.ToArray(),48)-ne0x497230){throw 'Relocation differs'}
[IO.File]::WriteAllText("$out/original.h",$h)
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC';$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH";$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($name in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($name,$null,'Process')}
$names=@('bh_route_choice');$objects=@()
foreach($name in ($names+@('ai_decisions_original'))){
 $folder=if($name-eq'ai_decisions_original'){'tests'}else{'src'}
 & "$root/analysis/toolchain-sp3/bin/CL.EXE" /nologo /c /ML /O2 /G5 /Oi /Gy "/Fo$out/$name.obj" "$root/$folder/$name.c"
 if($LASTEXITCODE){throw 'Compile failed'};$objects+='"'+$out+'/'+$name+'.obj"'
}
$exclude='runtime_fixes.obj|"/out:|"/map:|'+(($names|ForEach-Object{$_+'\.obj'})-join'|')
$rsp=@(Get-Content "$root/build/runtime-fixes/link.rsp"|Where-Object{$_-notmatch$exclude})
$rsp+=$objects;$rsp+='"/out:'+$out+'/ai-decisions.exe"'
[IO.File]::WriteAllLines("$out/link.rsp",$rsp)
& "$vc/bin/LINK.EXE" "@$out/link.rsp";if($LASTEXITCODE){throw 'Link failed'}
Push-Location $root
try{& "$out/ai-decisions.exe"|Tee-Object -FilePath "$out/result.log";$result=$LASTEXITCODE}finally{Pop-Location}
exit $result
