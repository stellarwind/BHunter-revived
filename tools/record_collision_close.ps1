# Durable evidence only; never executes the game or original image.
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out="$root/META/m4/playable/collision-close"
function Hash([string]$path){return (Get-FileHash $path).Hash.ToLower()}
$native=Get-Content "$out/traversal/validation.json" -Raw|ConvertFrom-Json
$cache=Get-Content "$out/cache/native/validation.json" -Raw|ConvertFrom-Json
$graph=Get-Content "$out/cache/graphs.json" -Raw|ConvertFrom-Json
$candidate=Get-Content "$out/traversal/candidate-graph.json" -Raw|ConvertFrom-Json
$linked=Get-Content "$out/native-linked.json" -Raw|ConvertFrom-Json
$game=Get-Content "$root/build/game/build.json" -Raw|ConvertFrom-Json
$checks=@(Get-Content "$out/source-checks.log"|Where-Object{$_.StartsWith('{')}|ForEach-Object{$_|ConvertFrom-Json})
if(!$native.passed-or!$cache.native_result-or!$graph.passed-or!$candidate.passed){throw 'Missing passing original-backed evidence'}
foreach($p in $cache.native_result.PSObject.Properties){if($p.Name-ne'cases'-and$p.Value-ne0){throw 'Cache native differences'}}
if($linked.executable_sha256-ne(Hash "$root/$($native.target)")){throw 'Stale native link proof'}
if($graph.object_sha256-ne(Hash "$root/build/collision-full-original/bh_collision_cache.obj")){throw 'Stale cache graph'}
if($candidate.object_sha256-ne(Hash "$root/build/collision-context-probe/query.obj")){throw 'Stale candidate graph'}
if($native.native.differences-or$native.native.state_differences-or@($checks|Where-Object{$_.failures}).Count){throw 'Comparison failures'}
if((Hash "$root/build/game/BHunter-source.exe")-ne$game.executable_sha256){throw 'Shipping image changed'}
$sourceHashes=[ordered]@{}
$paths=@('src/bh_collision_cache.c','src/bh_collision_cache.h','src/bh_collision_query.c','src/bh_collision_query.h',
 'src/bh_collision_core_model.h','src/bh_collision_model.c','src/bh_bsp.c','src/bh_math.c','src/bh_round.c',
 'tests/collision_cache_match.c','tests/collision_full_original.c','tools/check_collision_cache_match.ps1',
 'tools/check_collision_full_original.ps1','tools/compare_collision_cache_graph.py',
 'tools/compare_collision_traversal_candidate.py','tools/audit_ai_linked.py','tools/record_collision_close.ps1')
New-Item -ItemType Directory -Force "$out/sources"|Out-Null
foreach($path in $paths){
 $hash=Hash "$root/$path";$sourceHashes[$path]=$hash
 Copy-Item -LiteralPath "$root/$path" -Destination "$out/sources/$hash-$([IO.Path]::GetFileName($path)).txt"
 if($path.StartsWith('src/')-and$native.sources.$path-and$native.sources.$path-ne$hash){throw 'Native source variant differs'}
}
$reference=Hash "$root/GAME_DATA/BHUNTER.EXE"
if($reference-ne'c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'){throw 'Original changed'}
$cacheScope=@(
 'Complete original423B/152-instruction cache owner, successful allocation and already-prepared paths; PC53 nearest masked finite midpoint/radius intermediates',
 'Decoded positive counts1..65535; live disjoint immutable node/face inputs and output arrays; five successful allocator calls do not inspect pending model publication',
 'Actual source and original allocation sizes/order, face words/runtime reset and all center/squared-radius/radius expression graphs agree',
 'Actual round helper11B interpreted; binary64 scratch spills are identity atPC53; only individual finite add/mul operands commute; no reassociation',
 'Reviewed count induction uses pinned actual node/face tails: initial0, +1 index, +12 center/face stride, +56 node stride; signed/unsigned comparisons agree in the admitted count range',
 'Original lazy guard and source same-BSP prepared guard preserve mutable arrays; argument checks, allocation rollback, transactional publication and source owner layout are adapters')
$cacheLimits=@('No raw/relocation-normalized byte match; complete cache candidate remains uninstalled',
 'PC24/PC64 native cases are supporting observations only; runtime FP transitions, NaN payloads and exception flags are unproved',
 'Custom allocator observation/reentrancy, failing allocations in the original and arbitrary malformed inputs are outside the Close domain',
 'Cache variant is linked/exercised in native source targets and stripped from the shipping image; no shipping cache-consumer equivalence inferred')
$traversalLimits=@('Production iterative execute/mark_faces/stack-continuation refinement lacks complete instruction-graph proof; remains Functional',
 '32768 native cases use complete original391B source traversal and2591B destination traversal, with actual source math shared as an explicit dependency contract',
 'Uninstalled recursive candidate has147 complete instructions equivalent under one body EBX/EDI bijection and unchanged ABI save/restore slots; proof never transfers to iterative production code',
 'Arbitrary topology, all contact degeneracies and runtime precision transitions remain open; no whole-family1:1 claim')
$validation=[ordered]@{date='2026-10-29';passed=$true;state='cache_scoped_close_traversal_functional_with_new_original_backed_evidence';family_counts=@{owners=18;exact=3;close_scoped=1;functional_proof_open=14};sources=$sourceHashes;original_exe_sha256=$reference;cache_scope=$cacheScope;cache_limits=$cacheLimits;traversal_limits=$traversalLimits;cache_native=$cache.native_result;cache_graph=@{original_bytes=423;original_instructions_covered=$graph.original_covered.Count;source_bytes=$graph.source_bytes;negative_controls=$graph.negative_controls;count_recurrence_review=$graph.count_recurrence_review};traversal_native=$native.native;source_checks=$checks;shipping_executable_sha256=$game.executable_sha256;shipping_rebuilt=$false;production_sources_changed=$false;native_source_link='native-linked.json';original_adapter_review=@{complete_owners=@('0044d2f0:391','0044c200:2591','0044b130:423');mapper_or_game_loader=$false;scope='Only verified closed owners with reviewed calls/globals/constants rebound in test executables';initial_rejected_setup='Initial traversal adapter missed immediate relative-matrix address operands; corrected operand inventory and rerun. Failed setup is excluded, not a production-code discrepancy.'};byte_matching=@{question='Does the real surrounding C TU context resolve retained cache/traversal candidate discrepancies?';positive_control='Current exact87B geometry vertex core checked from actual COFF against immutable EXE';context_probe='build/collision-context-probe';cache_exact=$false;traversal_exact=$false;stopping_condition='Context change did not resolve cache instruction/spill differences or traversal EBX/EDI allocation; stop rather than change global flags or install candidate globals'};game_executed=$false;renderer_executed=$false;python_integration_tests_authored=$false;agents_used=$false}
[IO.File]::WriteAllText("$out/validation.json",($validation|ConvertTo-Json -Depth 30)+"`n")
$ledgerPath="$root/META/function-status.json";$text=[IO.File]::ReadAllText($ledgerPath)
if($text.Contains('"collision-cache-scoped-close-20261029"')){throw 'Preserve recorded ledger variants; audit already recorded'}
$ledger=$text|ConvertFrom-Json
$records=@()
foreach($address in @('0044b130','0044d2f0','0044c200')){
 $prior=@($ledger.records|Where-Object{$_.id-eq"collision-audit-$address-20261029"})[0]
 if(!$prior){throw 'Missing previous family audit'}
 $isCache=$address-eq'0044b130'
 $record=[ordered]@{id=$(if($isCache){'collision-cache-scoped-close-20261029'}else{"collision-traversal-$address-original-audit-20261029"});original_va=$address;function=$prior.function;role=$prior.role;provenance_confidence=$prior.provenance_confidence;implementation='func';integration=$(if($isCache){'linked_harness'}else{'linked_game'});source=$prior.source;source_sha256=$sourceHashes.($prior.source);target=$(if($isCache){$native.target}else{'build/game/BHunter-source.exe'});supersedes=$prior.id;validation=@{evidence='META/m4/playable/collision-close/validation.json';classification=$(if($isCache){'Close (scoped PC53 successful finite-input/lazy-cache contract)'}else{'Functional; complete iterative refinement graph open'});close_scoped=[bool]$isCache;raw_byte_match=$false;original_code_in_production=$false;scope=$(if($isCache){$cacheScope}else{$traversalLimits});limits=$(if($isCache){$cacheLimits}else{$traversalLimits})};evidence=@('META/m4/playable/collision-close/validation.json','META/m4/playable/collision-close/native-linked.json')+@($prior.evidence)}
 $records+=$record
}
$records+=@{id='collision-full-original-adapter-20261029';role='engine_candidate';implementation='adapter';integration='linked_harness';source=@('tests/collision_full_original.c','tests/collision_cache_match.c','tools/check_collision_full_original.ps1','tools/check_collision_cache_match.ps1');target=$native.target;game_admitted=$false;validation=@{original_code_executed=$true;production_uses_original_code=$false;evidence='META/m4/playable/collision-close/validation.json';scope='Explicit test-only closed original traversal/cache bodies; no general loader, game image mapping, native gameplay or renderer execution'}}
[IO.File]::WriteAllText("$out/ledger-fragment.json",($records|ConvertTo-Json -Depth 30)+"`n")
$fragment=($records|ConvertTo-Json -Depth 30).Trim();$contents=$fragment.Substring(1,$fragment.Length-2).Trim()
$anchor='"records": [';$at=$text.IndexOf($anchor);if($at-lt0){throw 'Missing records array'}
$updated=$text.Insert($at+$anchor.Length,"`n"+$contents+",");$null=$updated|ConvertFrom-Json
[IO.File]::WriteAllText($ledgerPath,$updated)
Write-Output 'Cache scoped Close recorded. Production traversal remains Functional; native and uninstalled-candidate proofs kept separate.'
