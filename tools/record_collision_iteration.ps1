# Record the current production proof, preserving earlier candidate evidence.
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out="$root/META/m4/playable/collision-iteration"
function Hash([string]$path){return (Get-FileHash $path).Hash.ToLower()}
$proof=Get-Content "$out/continuations.json" -Raw|ConvertFrom-Json
$controls=Get-Content "$out/controls.json" -Raw|ConvertFrom-Json
$native=Get-Content "$out/native/validation.json" -Raw|ConvertFrom-Json
$production=Get-Content "$out/production-linked.json" -Raw|ConvertFrom-Json
$nativeLinked=Get-Content "$out/native-linked.json" -Raw|ConvertFrom-Json
$positive=Get-Content "$out/exact-positive-control.json" -Raw|ConvertFrom-Json
$game=Get-Content "$root/build/game/build.json" -Raw|ConvertFrom-Json
if(!$proof.passed-or!$controls.passed-or!$native.passed-or!$proof.native_code_equivalence.passed-or!$positive.matched){throw 'Passing proof/control/native/link evidence required'}
if($proof.candidate_read){throw 'Candidate evidence must not enter production admission'}
if($proof.object_sha256-ne(Hash "$root/build/game/bh_collision_query.obj")){throw 'Production object changed'}
if($production.executable_sha256-ne(Hash "$root/build/game/BHunter-source.exe")-or$production.executable_sha256-ne$game.executable_sha256){throw 'Stale shipping binding proof'}
if($nativeLinked.executable_sha256-ne(Hash "$root/$($native.target)")-or$nativeLinked.executable_sha256-ne$native.executable_sha256){throw 'Stale native binding proof'}
if($proof.native_code_equivalence.native_object_sha256-ne(Hash "$root/build/collision-full-original/bh_collision_query.obj")){throw 'Native query variant changed'}
foreach($p in $proof.obligations){
 if($p.result-ne'unsat'-or!$p.nonvacuous-or!$p.smt_file-or(Hash "$out/$($p.smt_file)")-ne$p.smt_sha256){throw 'Missing or stale replayable SMT obligation'}
}
$checks=@(Get-Content "$out/source-checks.log"|Where-Object{$_.StartsWith('{')}|ForEach-Object{$_|ConvertFrom-Json})
if(@($checks|Where-Object{$_.failures}).Count-or$native.native.differences-or$native.native.state_differences){throw 'Current comparison failure'}
$sourceBuild=Get-Content "$root/build/collision-audit-source/validation.json" -Raw|ConvertFrom-Json
foreach($p in $sourceBuild.sources.PSObject.Properties){if((Hash "$root/$($p.Name)")-ne$p.Value){throw "Source-check snapshot differs: $($p.Name)"}}
$hashes=[ordered]@{}
$paths=@('src/bh_collision_query.c','src/bh_collision_query.h','src/bh_collision_model.c','src/bh_math.c','src/bh_bsp.c',
 'src/bh_collision_core_model.h','tests/collision_full_original.c','tools/prove_collision_iteration.py',
 'tools/check_collision_iteration_controls.ps1','tools/check_collision_full_original.ps1',
 'tools/audit_ai_linked.py','tools/record_collision_iteration.ps1')
New-Item -ItemType Directory -Force "$out/sources"|Out-Null
foreach($path in $paths){
 $hash=Hash "$root/$path";$hashes[$path]=$hash
 Copy-Item -LiteralPath "$root/$path" -Destination "$out/sources/$hash-$([IO.Path]::GetFileName($path)).txt"
 if($native.sources.$path-and$native.sources.$path-ne$hash){throw 'Native/production source disagreement'}
}
$reference=Hash "$root/GAME_DATA/BHUNTER.EXE"
if($reference-ne'c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'){throw 'Original changed'}
$scope=@(
 'Production bh_collision_query_run/execute/mark_faces/leaf/step, refined directly against immutable0044d2f0; no uninstalled candidate is read or admitted',
 'Finite PC53 nearest masked arithmetic; direct original/production retained sphere predicate graphs agree without reassociation, with the same logical selected bound fields and transformed float32 point',
 'Valid nonaliasing source owner/field views, immutable finite geometry, clean initial suppression, acyclic node/list topology and adequate workspace; query remains within1048576 work increments',
 'Arbitrary bounded node/depth/face/counter/next values in actual x86 local transitions. Reviewed finite-list and activation-stack induction composes solver lemmas; not a sampled whole-program or automated termination claim',
 'Source phases0/1/2 preserve original enter/front-return/back-return continuations; negative calls stutter, front hit skips back, and hit unwind has no further destination observations',
 'Both ordered leaf lists, repeated identities/shared tails, per-face increments/decrements and original child-result preservation refine source suppression; bulk root cleanup matches complete original unwind from zero',
 'Actual wrapper forwards all arguments, finite/owner guards remain explicit adapters, root starts phase0, and source output changes only for a hit')
$limits=@(
 '0044c200 destination/contact and matrix-transform implementations remain separately declared deterministic memory-preserving dependency contracts with corresponding contact payloads; this does not promote their full fidelity',
 'Source argument/topology/work/capacity/error checks, explicit field views, work counters and output/ownership layout are adapters; malformed/cyclic/over-budget original behavior is outside Close',
 'SMT uses normalized disjoint address regions and byte-derived32-bit instructions; valid physical field-view translation is a contract, not arbitrary aliasing equivalence',
 'Runtime PC24/PC64 transitions, NaN payloads, exceptional intermediates and FP exception flags remain unproved',
 'No byte match: original391B global two-index owner differs from production41B public adapter,654B iterative execute and176B mark helper')
$validation=[ordered]@{date='2026-10-29';passed=$true;state='production_source_subtree_traversal_scoped_close';family_counts=@{owners=18;exact=3;close_scoped=2;functional_proof_open=13};owner='0044d2f0';implementation='func';classification='Close (scoped production iterative refinement)';scope=$scope;limits=$limits;sources=$hashes;original_exe_sha256=$reference;production_object_sha256=$proof.object_sha256;production_executable_sha256=$game.executable_sha256;production_rebuilt=$false;production_sources_changed=$false;solver=@{package='z3-solver==4.13.3.0';version=$proof.z3_version;native_dll_sha256=Hash "$root/build/proof-deps/z3/lib/libz3.dll";nonvacuous_unsat_obligations=$proof.obligations.Count;replayable_queries='continuations.json -> smt_file and smt_sha256';guarded_store_forwarding_queries=$proof.guarded_store_forwarding_queries;proof='continuations.json';negative_controls='controls.json'};native=@{evidence='native/validation.json';result=$native.native;linked='native-linked.json';same_production_functions_and_data=$proof.native_code_equivalence.complete_functions_and_referenced_data};source_checks=$checks;source_check_executable_sha256=Hash "$root/build/collision-audit-source/physics-source-checks.exe";production_link='production-linked.json';candidate_evidence_used=$false;byte_matching=@{question='Can the current production iterative owner byte-match original391B without changing its ABI, stack continuation representation or guards?';positive_control='Current87B production geometry vertex exact comparison';original_bytes=391;public_adapter_bytes=41;execute_bytes=654;mark_bytes=176;feasible_near_match=$false;stopping_condition='Different complete owner structure and ABI; no instruction-order/flag-only route. Keep production implementation and stop, without importing candidate globals.'};game_executed=$false;renderer_executed=$false;python_integration_tests_authored=$false;agents_used=$false;reproduce=@('python -m pip install --target build/proof-deps z3-solver==4.13.3.0','pwsh -NoProfile -File tools/check_collision_full_original.ps1','python tools/prove_collision_iteration.py --native-object build/collision-full-original/bh_collision_query.obj --dump-smt','pwsh -NoProfile -File tools/check_collision_iteration_controls.ps1')}
[IO.File]::WriteAllText("$out/validation.json",($validation|ConvertTo-Json -Depth 25)+"`n")
$ledgerPath="$root/META/function-status.json";$text=[IO.File]::ReadAllText($ledgerPath)
if($text.Contains('"collision-production-iteration-close-20261029"')){throw 'Preserve immutable audit records; current admission already recorded'}
$records=@(
 [ordered]@{id='collision-production-iteration-close-20261029';original_va='0044d2f0';function='bh_collision_query_run / production execute';role='engine_candidate';provenance_confidence='Complete ordinary original source-subtree traversal; no verified CRT/library/import/compiler-helper/shared-tail attribution';implementation='func';integration='linked_game';source='src/bh_collision_query.c';source_sha256=$hashes['src/bh_collision_query.c'];target='build/game/BHunter-source.exe';supersedes='collision-traversal-0044d2f0-original-audit-20261029';validation=@{classification='Close (scoped production iterative refinement)';close_scoped=$true;raw_byte_match=$false;original_code_in_production=$false;evidence='META/m4/playable/collision-iteration/validation.json';scope=$scope;limits=$limits};evidence=@('META/m4/playable/collision-iteration/continuations.json','META/m4/playable/collision-iteration/controls.json','META/m4/playable/collision-iteration/production-linked.json','META/m4/playable/collision-iteration/native/validation.json')},
 [ordered]@{id='collision-iteration-original-adapter-20261029';role='engine_candidate';implementation='adapter';integration='linked_harness';source=@('tests/collision_full_original.c','tools/check_collision_full_original.ps1');source_sha256=@{'tests/collision_full_original.c'=$hashes['tests/collision_full_original.c'];'tools/check_collision_full_original.ps1'=$hashes['tools/check_collision_full_original.ps1']};target=$native.target;game_admitted=$false;validation=@{original_code_executed=$true;production_uses_original_code=$false;scope='Complete391B source and2591B destination original traversal bodies with verified explicit globals/calls/constants and shared actual source math; no game loader or candidate';evidence='META/m4/playable/collision-iteration/native/validation.json'}}
)
$fragment=($records|ConvertTo-Json -Depth 25).Trim();[IO.File]::WriteAllText("$out/ledger-fragment.json",$fragment+"`n")
$anchor='"records": [';$at=$text.IndexOf($anchor);if($at-lt0){throw 'Ledger records missing'}
$updated=$text.Insert($at+$anchor.Length,"`n"+$fragment.Substring(1,$fragment.Length-2).Trim()+",")
$null=$updated|ConvertFrom-Json;[IO.File]::WriteAllText($ledgerPath,$updated)
Write-Output 'Production0044d2f0 scoped Close recorded; destination0044c200 and uninstalled candidate evidence remain separate.'
