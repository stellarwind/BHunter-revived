param([string]$EvidenceDirectory='META/m4/playable/collision-close/cache/native')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out=Join-Path $root 'build/collision-cache-match'
$evidence=Join-Path $root $EvidenceDirectory
$candidateFolder=Join-Path $root 'META/m4/playable/physics-byte-match/cache'
New-Item -ItemType Directory -Force $out,$evidence|Out-Null
$reference=Join-Path $root 'GAME_DATA/BHUNTER.EXE'
if((Get-FileHash $reference).Hash-ne'C86205E299119A66E27F687679EE7E978F26FA9A6C0BFB21F664AA6E74E848B1'){throw 'Reference differs'}
$bytes=[IO.File]::ReadAllBytes($reference)
function Raw([int]$address,[int]$length){
 $offset=if($address-lt0x489000){$address-0x400c00}else{$address-0x489000+555520}
 return ,[byte[]]$bytes[$offset..($offset+$length-1)]
}
$start=0x44b130;$length=423;$body=Raw $start $length
$rows=Get-Content "$root/META/m4/collision-cache/original/0044b130.disasm.json" -Raw|ConvertFrom-Json
$header=@('static void install_original_cache(unsigned char *p){unsigned long address;',('{unsigned char bytes[]={'+($body-join',')+'};memcpy(p,bytes,sizeof(bytes));}'))
$cursor=$start;$calls=0;$constants=0
foreach($row in $rows){
 $at=[Convert]::ToInt32($row.address,16);$data=[Convert]::FromHexString($row.bytes)
 if($at-ne$cursor-or[Convert]::ToHexString((Raw $at $data.Length))-ne$row.bytes.ToUpper()){throw 'Original instruction gap/mismatch'}
 $cursor+=$data.Length
 if($row.mnemonic-eq'CALL'){
  if($row.operands[0]-ne'0x0047dae0'-or$data.Length-ne5-or$data[0]-ne0xe8){throw 'Unreviewed original callee'}
  $header+=('address=(unsigned long)malloc-(unsigned long)(p+'+($at-$start+5)+');memcpy(p+'+($at-$start+1)+',&address,4);');++$calls
 }elseif($row.mnemonic-match'^J'){
  $target=[Convert]::ToInt32($row.operands[0].Substring(2),16)
  if($target-lt$start-or$target-ge$start+$length){throw 'External original branch'}
 }
 foreach($operand in $row.operands){
  if($operand-match'ptr \[0x(00489[0-9a-f]+)\]'){
   $va=[Convert]::ToInt32($Matches[1],16)
   if($va-ne0x489e98-or$operand-notmatch'^double'-or$data.Length-ne6-or[BitConverter]::ToInt32($data,2)-ne$va){throw 'Unreviewed original constant'}
   $constant=Raw $va 8
   if([BitConverter]::ToDouble($constant,0)-ne0.5){throw 'Original half constant differs'}
   $header+=('{unsigned char bytes[]={'+($constant-join',')+'};memcpy(p+1024,bytes,sizeof(bytes));}')
   $header+=('address=(unsigned long)(p+1024);memcpy(p+'+($at-$start+2)+',&address,4);');++$constants
  }
 }
}
if($cursor-ne$start+$length-or$calls-ne5-or$constants-ne3){throw 'Incomplete original owner closure'}
$header+='}';[IO.File]::WriteAllLines("$out/original-cache.h",$header)
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC';$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH"
$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($name in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($name,$null,'Process')}
$commands=@()
foreach($pair in @(@('META/m4/playable/physics-byte-match/cache/candidate.c','cache'),@('src/bh_collision_cache.c','production'),@('src/bh_round.c','round'),@('tests/collision_cache_match.c','native'))){
 $argv=@('/nologo','/c','/ML','/O2','/G5','/Oi','/Gy',"/I$out","/I$candidateFolder","/I$root/src","/Fo$out/$($pair[1]).obj","$root/$($pair[0])")
 if($pair[1]-eq'cache'){$argv=@('/Dbh_collision_cache_prepare=bh_collision_cache_prepare_candidate_wrapper','/Dbh_collision_cache_dispose=bh_collision_cache_dispose_candidate_wrapper')+$argv}
 $commands+=@{program='analysis/toolchain-sp3/bin/CL.EXE';arguments=$argv}
 & "$root/analysis/toolchain-sp3/bin/CL.EXE" @argv
 if($LASTEXITCODE){throw 'Candidate full-TU/native compile failed'}
}
$link=@('/nologo','/opt:ref','kernel32.lib',"$out/cache.obj","$out/production.obj","$out/round.obj","$out/native.obj","/out:$out/collision-cache-match.exe","/map:$out/collision-cache-match.map")
$commands+=@{program="$vc/bin/LINK.EXE";arguments=$link}
& "$vc/bin/LINK.EXE" @link
if($LASTEXITCODE){throw 'Native real-definition link failed'}
& "$out/collision-cache-match.exe"|Tee-Object "$out/native-result.log"
if($LASTEXITCODE){throw 'Native control failed'}
python "$root/tools/compare_ai_object.py" "$out/cache.obj" _bh_collision_cache_prepare_core 0x44b130 423 --bind _malloc=0x47dae0 --output "$evidence/object.json"
if($LASTEXITCODE){throw 'Object comparison failed'}
$listing=& llvm-objdump -dr --disassemble-symbols=_bh_collision_cache_prepare_core --x86-asm-syntax=intel "$out/cache.obj"
[IO.File]::WriteAllLines("$evidence/generated.asm",$listing)
$returnLine=$listing|Where-Object{$_-match'^\s*([0-9a-f]+):\s+c3\s+ret\s*$'}|Select-Object -First 1
if(!$returnLine-or$returnLine-notmatch'^\s*([0-9a-f]+):'){throw 'Complete candidate RET not found'}
$bodyLength=[Convert]::ToInt32($Matches[1],16)+1
$sourcePaths=@('META/m4/playable/physics-byte-match/cache/candidate.c','META/m4/playable/physics-byte-match/cache/bh_collision_cache_core.inc','META/m4/playable/physics-byte-match/cache/bh_collision_cache_core.h','src/bh_collision_core_model.h','src/bh_collision_cache.c','tests/collision_cache_match.c','tools/check_collision_cache_match.ps1')
$hashes=@{};foreach($path in $sourcePaths){$hashes[$path]=(Get-FileHash "$root/$path").Hash.ToLower()}
$object=Get-Content "$evidence/object.json" -Raw|ConvertFrom-Json
$record=@{date='2026-10-28';address='0044b130';role='engine_candidate';boundary='0044b130..0044b2d6 complete423-byte ordinary collision cache owner; no shared tail/compiler support';implementation='wip';installed=$false;target='build/collision-cache-match/collision-cache-match.exe';integration='linked_harness candidate full TU';sources=$hashes;commands=$commands;reference_sha256=(Get-FileHash $reference).Hash.ToLower();object_sha256=(Get-FileHash "$out/cache.obj").Hash.ToLower();executable_sha256=(Get-FileHash "$out/collision-cache-match.exe").Hash.ToLower();byte_match=$object.matched;original_body_length=423;generated_body_length=$bodyLength;native_stdout=(Get-Content "$out/native-result.log" -Raw);native_result=(Get-Content "$out/native-result.log" | Select-Object -Last 1)|ConvertFrom-Json;original_adapter=@{calls=$calls;callee='0047dae0 verified CRT malloc wrapper ->0047db00 verified __nh_malloc';provenance=@('META/m1/role-catalog.json','analysis/crt-comparison.json','META/m4/playable/physics-byte-match/cache/malloc-disasm.json');constants=$constants;original_code_required=$true;scope='Only complete423-byte owner copied into isolated native test with5 malloc calls rebound to actual CRT malloc and3 exact read-only0.5 operands relocated. No game loader.'};admission='Not admitted: full instruction comparison remains different and strict complete FP/spill graph equivalence is unestablished. No scheduling-only claim. Production cache source restored unchanged.';limits=@('No production allocation-plan binding installed','Candidate successful malloc path only; original unchecked allocation behavior is not a source failure contract','Native finite cases are diagnostic and not whole-physics fidelity evidence','No byte evidence transfers to existing source wrapper or other variants')}
$record|ConvertTo-Json -Depth 12|Set-Content "$evidence/validation.json"
Write-Output 'Cache candidate remains uninstalled; complete original/native discrepancy evidence retained.'
