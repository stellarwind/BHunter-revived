$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent;$out="$root/build/collision-traversal-match";$evidence="$root/META/m4/playable/physics-byte-match/traversal"
New-Item -ItemType Directory -Force $out,$evidence|Out-Null
$reference="$root/GAME_DATA/BHUNTER.EXE"
if((Get-FileHash $reference).Hash-ne'C86205E299119A66E27F687679EE7E978F26FA9A6C0BFB21F664AA6E74E848B1'){throw 'Original differs'}
$bytes=[IO.File]::ReadAllBytes($reference)
function Raw([int]$va,[int]$size){$offset=$va-0x400c00;return ,([byte[]]$bytes[$offset..($offset+$size-1)])}
$body=Raw 0x44d2f0 391;$transform=Raw 0x4012e0 116
$header=@('static void install_original_traversal(unsigned char *p){unsigned long address;',('{unsigned char bytes[]={'+($body-join',')+'};memcpy(p,bytes,sizeof(bytes));}'),('{unsigned char bytes[]={'+($transform-join',')+'};memcpy(p+1024,bytes,sizeof(bytes));}'))
$cursor=0x44d2f0;$bindings=@();$calls=@()
foreach($row in(Get-Content "$root/META/m4/collision-traversal/original/0044d2f0.disassembly.json" -Raw|ConvertFrom-Json)){
 $at=[Convert]::ToInt32($row.address,16);$data=[Convert]::FromHexString($row.bytes)
 if($at-ne$cursor-or[Convert]::ToHexString((Raw $at $data.Length))-ne$row.bytes.ToUpper()){throw 'Incomplete original instruction body'}
 $cursor+=$data.Length
 if($row.mnemonic-eq'CALL'){
  $target=[Convert]::ToInt32($row.operands[0].Substring(2),16)
  $bound=switch($target){0x4012e0{'p+1024'}0x44c200{'(unsigned char *)bh_traversal_node'}0x44d2f0{'p'}default{throw 'Unreviewed original callee'}}
  $header+=('address=(unsigned long)('+$bound+')-(unsigned long)(p+'+($at-0x44d2f0+5)+');memcpy(p+'+($at-0x44d2f0+1)+',&address,4);')
  $calls+=@{offset=$at-0x44d2f0;original_target=$target;binding=$bound}
 }elseif($row.mnemonic-match'^J'){
  $target=[Convert]::ToInt32($row.operands[0].Substring(2),16)
  if($target-lt0x44d2f0-or$target-ge0x44d477){throw 'External original branch'}
 }else{
  foreach($operand in $row.operands){
   if($operand-match'0x(0*4e[0-9a-f]+)'){
    $va=[Convert]::ToInt32($Matches[1],16)
    $bound=switch($va){0x4e0fcc{'&bh_traversal_a'}0x4e0fd0{'&bh_traversal_b'}0x4e1010{'&bh_traversal_relative'}default{throw 'Unreviewed original global'}}
    if([BitConverter]::ToInt32($data,$data.Length-4)-ne$va){throw 'Original global encoding differs'}
    $header+=('address=(unsigned long)('+$bound+');memcpy(p+'+($at-0x44d2f0+$data.Length-4)+',&address,4);')
    $bindings+=@{offset=$at-0x44d2f0;original_va=$va;binding=$bound}
   }
  }
 }
}
if($cursor-ne0x44d477-or$calls.Count-ne4-or$bindings.Count-ne6){throw 'Original closure incomplete'}
$disassembly=& llvm-objdump -d --start-address=0x4012e0 --stop-address=0x401354 --x86-asm-syntax=intel $reference
if($LASTEXITCODE-or(($disassembly-join"`n")-match'\s(call|j[a-z]+)\s')){throw 'Original transform not a closed leaf'}
$header+='}';[IO.File]::WriteAllLines("$out/original-traversal.h",$header)
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC';$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH";$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($name in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($name,$null,'Process')}
$sources=@('META/m4/playable/physics-byte-match/traversal/traversal-candidate.c','tests/collision_traversal_match.c','src/bh_round.c','src/bh_collision_query.c','src/bh_math.c','src/bh_fp.c','src/bh_collision_model.c','src/bh_bsp.c','src/bh_geometry.c','src/bh_vfile.c');$objects=@();$hashes=@{}
$baseline="$root/META/m4/playable/physics-byte-match/traversal/baseline-query.c"
$baselineSource=[IO.File]::ReadAllText($baseline)
$baselineSource+="`nint __cdecl bh_collision_baseline_gate_check(BHCollisionQuery *q,long an,long bn,const BHMat34 *m){return spheres(q,an,bn,m);}`n"
[IO.File]::WriteAllText("$out/baseline-query.c",$baselineSource)
& "$root/analysis/toolchain-sp3/bin/CL.EXE" /nologo /c /ML /O2 /G5 /Oi /Gy /Dbh_collision_query_init=bh_baseline_query_init /Dbh_collision_query_dispose=bh_baseline_query_dispose /Dbh_collision_query_node=bh_baseline_query_node /Dbh_collision_query_run=bh_baseline_query_run "/I$root/src" "/Fo$out/baseline-query.obj" "$out/baseline-query.c"
if($LASTEXITCODE){throw 'Baseline real TU compile failed'}
$objects+="$out/baseline-query.obj";$hashes['META/m4/playable/physics-byte-match/traversal/baseline-query.c']=(Get-FileHash $baseline).Hash.ToLower()
foreach($source in $sources){
 $object="$out/$([IO.Path]::GetFileNameWithoutExtension($source)).obj";$objects+=$object
 $flags=@();if($source-eq'src/bh_math.c'){$flags+='/Dbh_mat34_transform_point=bh_source_transform_point_dependency'}
 & "$root/analysis/toolchain-sp3/bin/CL.EXE" /nologo /c /ML /O2 /G5 /Oi /Gy /DBH_COLLISION_QUERY_GATE_CHECK @flags "/I$root/src" "/Fo$object" "$root/$source"
 if($LASTEXITCODE){throw 'Candidate or native oracle compile failed'}
 $hashes[$source]=(Get-FileHash "$root/$source").Hash.ToLower()
}
$rsp=@('/nologo','/opt:ref','kernel32.lib')+$objects+@("/out:$out/collision-traversal-match.exe","/map:$out/collision-traversal-match.map")
[IO.File]::WriteAllLines("$out/link.rsp",@($rsp|ForEach-Object{'"'+$_+'"'}));& "$vc/bin/LINK.EXE" "@$out/link.rsp";if($LASTEXITCODE){throw 'Link failed'}
$result=& "$out/collision-traversal-match.exe";$status=$LASTEXITCODE;$result|Tee-Object "$out/result.log"
$hashes['src/bh_collision_query.c']=(Get-FileHash "$root/src/bh_collision_query.c").Hash.ToLower()
foreach($source in @('src/bh_collision_core_model.h','src/bh_collision_query.h','tools/check_collision_traversal_match.ps1')){$hashes[$source]=(Get-FileHash "$root/$source").Hash.ToLower()}
$assembly=& llvm-objdump -dr --disassemble-symbols=_bh_collision_traversal_core --x86-asm-syntax=intel "$out/traversal-candidate.obj";if($LASTEXITCODE){throw 'Candidate disassembly failed'}
$assembly|Set-Content "$evidence/candidate-assembly.txt"
$tools=@{};foreach($name in @('CL.EXE','C1.DLL','C2.EXE')){$path="$root/analysis/toolchain-sp3/bin/$name";if(Test-Path $path){$tools[$name]=(Get-FileHash $path).Hash.ToLower()}}
@{passed=($status-eq0);candidate_installed=$false;target='build/collision-traversal-match/collision-traversal-match.exe';target_sha256=(Get-FileHash "$out/collision-traversal-match.exe").Hash.ToLower();native=($result|ConvertFrom-Json);sources=$hashes;toolchain=$tools;compiler_flags='/nologo /c /ML /O2 /G5 /Oi /Gy /DBH_COLLISION_QUERY_GATE_CHECK';reproduce='pwsh -NoProfile -File tools/check_collision_traversal_match.ps1';original_owner=@{address='0044d2f0';length=391;raw_sha256=([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($body))).ToLower();relocated_calls=$calls;bound_verified_globals=$bindings};original_transform=@{address='004012e0';length=116;raw_sha256=([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($transform))).ToLower();closed_leaf=$true};limits=@('Uninstalled candidate is not a strict byte or instruction-order-only match','Destination callback is a test-only gate probe, not recovered0044c200','Actual baseline and corrected query TUs execute their real sphere helper through test-only guarded exports; corrected public query also runs atPC53','Original transform leaf is intentionally shared to isolate spheregate arithmetic; production source transform dependency remains independently unverified','No game image mapping or game execution; only complete isolated original traversal and transform leaves execute')}|ConvertTo-Json -Depth 10|Set-Content "$evidence/validation.json"
if($status){throw 'Native oracle failed'}
