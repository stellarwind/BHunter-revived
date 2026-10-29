$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out=Join-Path $root 'build/physics-rotation-match'
$evidence=Join-Path $root 'META/m4/playable/physics-byte-match/rotation'
$candidateSource='META/m4/playable/physics-byte-match/rotation/candidate.c'
New-Item -ItemType Directory -Force $out,$evidence|Out-Null
$reference=Join-Path $root 'GAME_DATA/BHUNTER.EXE'
if((Get-FileHash $reference).Hash-ne'C86205E299119A66E27F687679EE7E978F26FA9A6C0BFB21F664AA6E74E848B1'){throw 'Reference differs'}
$bytes=[IO.File]::ReadAllBytes($reference)
$header=@('static void install_original_rotation(unsigned char *p){')
$specs=@(@('x',0x401510,251),@('y',0x4016e0,247))
$originals=@{}
foreach($spec in $specs){
 $name=$spec[0];$address=$spec[1];$length=$spec[2]
 $body=[byte[]]$bytes[($address-0x400c00)..($address-0x400c00+$length-1)]
 $rows=Get-Content "$root/META/m4/physics-response/original/$('{0:x8}'-f $address).disassembly.json" -Raw|ConvertFrom-Json
 $cursor=$address
 foreach($row in $rows){
  $data=[Convert]::FromHexString($row.bytes);$at=[Convert]::ToInt32($row.address,16)
  if($at-ne$cursor-or[Convert]::ToHexString([byte[]]$bytes[($at-0x400c00)..($at-0x400c00+$data.Length-1)])-ne$row.bytes.ToUpper()){throw 'Original instruction gap/mismatch'}
  if($row.mnemonic-match'CALL|^J'){throw 'Unexpected original control dependency'}
  $cursor+=$data.Length
 }
 if($cursor-ne$address+$length){throw 'Original owner incomplete'}
 $originals[$name]=$rows
 $offset=if($name-eq'x'){0}else{4096}
 $header+=('{unsigned char b[]={'+($body-join',')+'};memcpy(p+'+$offset+',b,sizeof(b));}')
}
$header+='}';[IO.File]::WriteAllLines("$out/original-rotation.h",$header)
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC';$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH"
$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($name in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($name,$null,'Process')}
$commands=@()
foreach($pair in @(@($candidateSource,'rotation'),@('src/bh_round.c','round'),@('tests/physics_rotation_match.c','native'))){
 $argv=@('/nologo','/c','/ML','/O2','/G5','/Oi','/Gy',"/I$out","/I$root/src","/Fo$out/$($pair[1]).obj","$root/$($pair[0])")
 $commands+=@{program='analysis/toolchain-sp3/bin/CL.EXE';arguments=$argv}
 & "$root/analysis/toolchain-sp3/bin/CL.EXE" @argv
 if($LASTEXITCODE){throw 'Real source compile failed'}
}
& "$vc/bin/LINK.EXE" /nologo /opt:ref kernel32.lib "$out/rotation.obj" "$out/round.obj" "$out/native.obj" "/out:$out/physics-rotation-match.exe" "/map:$out/physics-rotation-match.map"
if($LASTEXITCODE){throw 'Native link failed'}
& "$out/physics-rotation-match.exe"|Tee-Object "$out/native-result.log"
if($LASTEXITCODE){throw 'Native original comparison failed'}

# Symbolically execute every straight-line x87 instruction, retaining rounding
# nodes and matrix-store order. No reassociation or cancellation is permitted.
function Execute-Rotation($rows){
 $stack=[Collections.Generic.List[string]]::new();$memory=@{};$writes=[Collections.Generic.List[string]]::new();$sp=0
 $memory['4']='ANGLE';$memory['8']='MATRIX'
 for($j=0;$j-lt12;++$j){$memory["m$($j*4)"]="M$j"}
 function Rounded([string]$v){if($v-match'^(M\d+|ANGLE|R32\(.*\))$'){return $v};return "R32($v)"}
 function Binary([string]$operation,[string]$left,[string]$right){
  # Operand exchange of one finite add/multiply has no changed arithmetic tree.
  if(!$script:strictOperandOrder-and$operation-in@('ADD','MUL')-and[string]::CompareOrdinal($left,$right)-gt0){$left,$right=$right,$left}
  return "$operation($left,$right)"
 }
 foreach($row in $rows){
  $mn=$row.mnemonic.ToUpper();$op=($row.operands-join',').ToLower().Replace(' ','').Replace('st0','st(0)')
  $op=[regex]::Replace($op,'st([1-7])','st($1)')
  $key=$null;$value=$null
  if($op-match'\[esp(?:\+0x([0-9a-f]+))?\]'){$offset=if($Matches[1]){[Convert]::ToInt32($Matches[1],16)}else{0};$key=[string]($sp+$offset)}
  elseif($op-match'\[eax(?:\+0x([0-9a-f]+))?\]'){$offset=if($Matches[1]){[Convert]::ToInt32($Matches[1],16)}else{0};$key="m$offset"}
  if($key-and$mn-notin@('FST','FSTP')){if(!$memory.ContainsKey($key)){throw "Undefined memory $key in $mn $op"};$value=$memory[$key]}
  $stIndex=if($op-match'st\(([0-7])\)'){[int]$Matches[1]}else{1}
  switch($mn){
   'PUSH' {if($op-ne'ecx'){throw 'Unexpected PUSH'};$sp-=4}
   'POP' {if($op-ne'ecx'){throw 'Unexpected POP'};$sp+=4}
   'MOV' {if($op-notmatch'^eax,dwordptr\[esp\+0xc\]$'-or$value-ne'MATRIX'){throw "Unexpected MOV $op"}}
   'NOP' {}
   'RET' {if($stack.Count-or$sp){throw 'Unbalanced function exit'}}
   'FLD' {if(!$key){if($op-notmatch'^st\([0-7]\)$'){throw "Unexpected FLD $op"};$value=$stack[$stIndex]};$stack.Insert(0,$value)}
   'FXCH' {$v=$stack[0];$stack[0]=$stack[$stIndex];$stack[$stIndex]=$v}
   'FSIN' {$stack[0]="SIN($($stack[0]))"}
   'FCOS' {$stack[0]="COS($($stack[0]))"}
   {$_-in@('FST','FSTP')} {
    if($key){$v=Rounded $stack[0];$memory[$key]=$v;if($key.StartsWith('m')){$writes.Add("$key=$v")}}
    elseif($op-ne'st(0)'){throw "Unexpected register store $op"}
    if($mn-eq'FSTP'){$stack.RemoveAt(0)}
   }
   {$_-in@('FMUL','FADD','FSUB','FSUBR')} {
    $right=if($key){$value}else{$stack[$stIndex]};$left=$stack[0]
    $operation=@{FMUL='MUL';FADD='ADD';FSUB='SUB';FSUBR='SUB'}[$mn]
    if($mn-eq'FSUBR'){$left,$right=$right,$left}
    $stack[0]=Binary $operation $left $right
   }
   {$_-in@('FMULP','FADDP','FSUBP','FSUBRP')} {
    $left=$stack[$stIndex];$right=$stack[0];$operation=@{FMULP='MUL';FADDP='ADD';FSUBP='SUB';FSUBRP='SUB'}[$mn]
    if($mn-eq'FSUBRP'){$left,$right=$right,$left}
    $stack[$stIndex]=Binary $operation $left $right;$stack.RemoveAt(0)
   }
   default {throw "Unsupported instruction $mn $op"}
  }
 }
 return ,$writes.ToArray()
}
function Get-RotationAudit($rows){
 $depth=0;$maximum=0;$histogram=@{};$scratch=@();$arithmetic=@()
 foreach($row in $rows){
  $mn=$row.mnemonic.ToUpper();$op=($row.operands-join',').ToLower()
  if(!$histogram.ContainsKey($mn)){$histogram[$mn]=0};++$histogram[$mn]
  if($mn-eq'FLD'){++$depth}
  elseif($mn-in@('FSTP','FADDP','FMULP','FSUBP','FSUBRP')){--$depth}
  $maximum=[Math]::Max($maximum,$depth)
  if($mn-in@('FST','FSTP')-and$op-match'\[esp'){$scratch+=@{opcode=$mn;operand=$op}}
  if($mn-match'^F(ADD|SUB|MUL)'){$arithmetic+=@{opcode=$mn;operand=$op}}
 }
 return @{instruction_count=$rows.Count;maximum_x87_stack_depth=$maximum;final_x87_stack_depth=$depth;mnemonic_counts=$histogram;scratch_stores=$scratch;arithmetic_instructions=$arithmetic}
}
function Read-PEBody([string]$path,[long]$address,[int]$length){
 $data=[IO.File]::ReadAllBytes($path);$pe=[BitConverter]::ToInt32($data,0x3c)
 $imageBase=[BitConverter]::ToUInt32($data,$pe+52);$count=[BitConverter]::ToUInt16($data,$pe+6)
 $sections=$pe+24+[BitConverter]::ToUInt16($data,$pe+20)
 for($index=0;$index-lt$count;++$index){
  $at=$sections+40*$index;$rva=[BitConverter]::ToUInt32($data,$at+12);$size=[BitConverter]::ToUInt32($data,$at+16);$disk=[BitConverter]::ToUInt32($data,$at+20)
  $offset=$address-$imageBase-$rva
  if($offset-ge0-and$offset+$length-le$size){return ,[byte[]]$data[($disk+$offset)..($disk+$offset+$length-1)]}
 }
 throw 'Linked body not found in PE section'
}
$proofs=@()
foreach($spec in $specs){
 $name=$spec[0];$symbol="_bh_mat34_rotate_world_$name"
 python "$root/tools/compare_ai_object.py" "$out/rotation.obj" $symbol ('0x{0:x}'-f$spec[1]) $spec[2] --output "$evidence/$name-object.json"
 if($LASTEXITCODE){throw 'Object comparison failed'}
 $listing=& llvm-objdump -d "--disassemble-symbols=$symbol" --x86-asm-syntax=intel "$out/rotation.obj"
 [IO.File]::WriteAllLines("$evidence/$name-generated.asm",$listing)
 $rows=@();$seenReturn=$false;$bodyLength=0
 foreach($line in $listing){
  if($seenReturn){continue}
  if($line-match'^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2}\s+)+)\s*([a-z]+)\s*(.*)$'){
   $bodyLength=[Convert]::ToInt32($Matches[1],16)+([Convert]::FromHexString(($Matches[2]-replace'\s',''))).Length
   $mn=$Matches[3];$operands=$Matches[4]-split',\s*';$rows+=@{mnemonic=$mn;operands=$operands}
   if($mn-eq'ret'){$seenReturn=$true}
  }
 }
 if(!$seenReturn){throw 'Generated owner missing RET'}
 $script:strictOperandOrder=$true
 $strictExpected=Execute-Rotation $originals[$name];$strictActual=Execute-Rotation $rows
 $strictDifferences=@()
 for($index=0;$index-lt8;++$index){if($strictExpected[$index]-ne$strictActual[$index]){$strictDifferences+=@{store=$index;original=$strictExpected[$index];generated=$strictActual[$index]}}}
 $script:strictOperandOrder=$false
 $expected=Execute-Rotation $originals[$name];$actual=Execute-Rotation $rows
 if(($expected-join';')-ne($actual-join';')){throw "Complete ordered symbolic output differs for $name"}
 if($actual.Count-ne8){throw 'Incomplete matrix stores'}
 # Execute a changed original subtraction instruction as the negative control.
 $mutation=@();$changed=$false
 foreach($row in $originals[$name]){
  $mn=$row.mnemonic
  if(!$changed-and$mn-eq'FSUBP'){$mn='FADDP';$changed=$true}
  $mutation+=@{mnemonic=$mn;operands=$row.operands}
 }
 $mutated=Execute-Rotation $mutation
 if(!$changed-or($mutated-join';')-eq($expected-join';')){throw 'Mutation negative control failed'}
 $object=Get-Content "$evidence/$name-object.json" -Raw|ConvertFrom-Json
 if($object.relocations.Count){throw 'Unexpected object relocation'}
 $mapline=Get-Content "$out/physics-rotation-match.map"|Where-Object{$_-match("\s"+[regex]::Escape($symbol)+"\s+([0-9a-fA-F]{8})\s")}|Select-Object -First 1
 if(!$mapline-or$mapline-notmatch("\s"+[regex]::Escape($symbol)+"\s+([0-9a-fA-F]{8})\s")){throw 'Missing linked owner'}
 $linkedVA=[Convert]::ToInt64($Matches[1],16);$linked=Read-PEBody "$out/physics-rotation-match.exe" $linkedVA $bodyLength
 if([Convert]::ToHexString($linked).ToLower()-ne$object.generated_hex.Substring(0,$bodyLength*2)){throw 'Linked owner differs from real TU object'}
 $proofs+=@{axis=$name;original_va=('{0:x8}'-f$spec[1]);original_length=$spec[2];generated_length=$bodyLength;original_audit=(Get-RotationAudit $originals[$name]);generated_audit=(Get-RotationAudit $rows);ordered_stores=$actual;complete_finite_commutative_symbolic_match=$true;strict_operand_order_match=($strictDifferences.Count-eq0);strict_operand_differences=$strictDifferences;sign_instruction_mutation_rejected=$true;byte_match=$object.matched;linked_va=('{0:x8}'-f$linkedVA);linked_complete_body_equals_object=$true;linked_body_sha256=([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($linked))).ToLower()}
}
$record=@{date='2026-10-28';installed=$false;admission='wip candidate; ordered arithmetic operands differ';sources=@{$candidateSource=(Get-FileHash "$root/$candidateSource").Hash.ToLower();'tests/physics_rotation_match.c'=(Get-FileHash "$root/tests/physics_rotation_match.c").Hash.ToLower();'tools/check_physics_rotation_match.ps1'=(Get-FileHash $PSCommandPath).Hash.ToLower()};commands=$commands;toolchain=@{compiler=(Get-FileHash "$root/analysis/toolchain-sp3/bin/CL.EXE").Hash.ToLower();math_header=(Get-FileHash "$sp/include/math.h").Hash.ToLower();linker=(Get-FileHash "$vc/bin/LINK.EXE").Hash.ToLower()};classification='engine_candidate; complete ordinary matrix rotation owners, no library identification or compiler scaffolding';target='build/physics-rotation-match/physics-rotation-match.exe';object_sha256=(Get-FileHash "$out/rotation.obj").Hash.ToLower();executable_sha256=(Get-FileHash "$out/physics-rotation-match.exe").Hash.ToLower();native_result=(Get-Content "$out/native-result.log" -Raw).Trim();proofs=$proofs;residual_differences=@('Same PUSH ECX / POP ECX / RET, unchanged cdecl interface and4-byte local stack allocation.','x87 register allocation changes FXCH/ST-index selection and duplicate FLD placement.','Original final FSTP ST0 discard is absent; generated live x87 values finish naturally at depth0.','Coefficient spills preserve binary32 COS and SIN nodes. Input-value scratch spills move among existing stack/argument slots; all spilled matrix values are already exact binary32 inputs, so reloads introduce no new rounding.','All8 binary32 matrix stores retain original order and complete arithmetic expression graphs, without reassociation. ADD/MUL operand exchange is explicitly canonicalized for finite operands; SUB direction is preserved.','Untouched matrix row has no stores. No control-word instruction, call, branch, global, constant or ABI/layout adapter is present.');scope='Uninstalled full-TU candidate. Complete owner matrix-store order and finite commutative arithmetic graphs agree, but strict ADD operand order differs for4 stores per axis. User scheduling-only admission criterion is NOT met. Register allocation, duplicate stack-value lifetimes and x87 scheduling also differ. No raw byte-match or production integration claim. No exception-status/NaN-payload equivalence claim. Native comparisons cover PC24/53/64; runtime precision transitions remain unverified.'}
$record|ConvertTo-Json -Depth 12|Set-Content "$evidence/validation.json"
Write-Output 'Uninstalled candidate: finite comparisons pass; strict operand graphs differ. Scheduling-only admission rejected.'
