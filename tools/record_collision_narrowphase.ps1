$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out="$root/META/m4/playable/collision-narrowphase"
function Hash([string]$path){return (Get-FileHash $path).Hash.ToLower()}
function Read([string]$path){return Get-Content $path -Raw|ConvertFrom-Json}
$native=Read "$out/final-native/validation.json";$baseline=Read "$out/baseline-control.json"
$graphs=Read "$out/original-cut-graphs.json";$shippingGraphs=Read "$out/shipping-cut-graphs.json";$controls=Read "$out/controls.json"
$production=Read "$out/production-linked.json";$nativeLink=Read "$out/native-linked.json"
$iteration=Read "$out/iteration-refresh/continuations.json";$iterationControls=Read "$out/iteration-refresh/controls.json"
$game=Read "$root/build/game/build.json";$source=Read "$root/build/collision-narrowphase-source/validation.json"
if(!$native.passed-or!$baseline.passed-or!$graphs.passed-or!$shippingGraphs.passed-or!$controls.passed-or!$source.passed-or!$game.passed-or!$iteration.passed-or!$iterationControls.passed){throw 'Passing independent evidence required'}
if($native.native.differences-or$native.native.cached_differences-or$native.native.state_differences){throw 'Native differences'}
if(!$baseline.result.differences-or!$baseline.result.cached_differences){throw 'Old source must fail independently'}
if($graphs.source_object_sha256-ne(Hash "$root/build/collision-full-original/bh_collision_model.obj")-or$shippingGraphs.source_object_sha256-ne(Hash "$root/build/game/bh_collision_model.obj")){throw 'Stale interpolation graph variant'}
if($iteration.object_sha256-ne(Hash "$root/build/game/bh_collision_query.obj")-or!$iteration.native_code_equivalence.passed){throw 'Stale continuation proof'}
if($production.executable_sha256-ne$game.executable_sha256-or$production.executable_sha256-ne(Hash "$root/build/game/BHunter-source.exe")){throw 'Stale shipping linkage'}
if($nativeLink.executable_sha256-ne$native.executable_sha256-or$native.executable_sha256-ne(Hash "$root/$($native.target)")){throw 'Stale native linkage'}
$hashes=[ordered]@{}
$paths=@('src/bh_collision_model.c','src/bh_collision_model.h','src/bh_collision_query.c','src/bh_collision_query.h',
 'src/bh_round.c','src/bh_math.c','tests/collision_full_original.c','tools/compare_collision_cut_graph.py',
 'tools/check_collision_full_original.ps1','tools/check_collision_cut_controls.ps1','tools/check_collision_cut_baseline.ps1',
 'tools/check_collision_iteration_controls.ps1','tools/prove_collision_iteration.py','tools/record_collision_narrowphase.ps1')
New-Item -ItemType Directory -Force "$out/sources"|Out-Null
foreach($path in $paths){
 $hash=Hash "$root/$path";$hashes[$path]=$hash
 Copy-Item -LiteralPath "$root/$path" -Destination "$out/sources/$hash-$([IO.Path]::GetFileName($path)).txt"
 if($native.sources.$path-and$native.sources.$path-ne$hash){throw 'Native source context changed'}
 if($source.sources.$path-and$source.sources.$path-ne$hash){throw 'Connected source context changed'}
}
$checks=@(Get-Content "$root/build/collision-narrowphase-source/result.log"|Where-Object{$_.StartsWith('{')}|ForEach-Object{$_|ConvertFrom-Json})
if(@($checks|Where-Object{$_.failures}).Count){throw 'Connected source failures'}
$reference=Hash "$root/GAME_DATA/BHUNTER.EXE"
if($reference-ne'c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'){throw 'Original changed'}
$limits=@(
 'Full destination/face-owner graph, arbitrary polygon topology/degeneracies and all interval/plane-evaluation numerics remain Functional with explicit proof gaps; this slice is not whole-owner Close',
 'Original/source tests deliberately share actual source math; matrix/cross-product equivalence to original implementations remains a separate dependency',
 'PC53 nearest masked finite profile only; PC24/64 transitions, exceptional intermediates and NaN/exception-flag behavior remain unresolved',
 'Native cached cases use selected original faces at the hit pose and a separated trial pose; these cases do not establish all possible cached-contact configurations',
 'No raw byte match, renderer/gameplay execution, Python integration-test authoring or agents')
$validation=[ordered]@{date='2026-10-29';passed=$true;state='original_backed_narrowphase_product_spill_correction';family_counts=@{owners=18;exact=3;close_scoped=2;functional_proof_open=13};production_sources_changed=$true;production_translation_units=$game.translation_units;shipping_executable_sha256=$game.executable_sha256;sources=$hashes;original_exe_sha256=$reference;discrepancy='Old common interpolation retained products that original second-cut code spills to binary32; produced tiny nonzero contact coordinates instead of original zero';correction=@('Both owners round d*previous for X/Y/Z in second cut','0044c200 inline variant rounds previous_distance*current Y/Z, retains X','0044cc20 standalone variant rounds previous_distance*current Z, retains X/Y','First cut remains retained arithmetic in both owners; two owning variants are explicit, not merged');original_instruction_proof=@{file='original-cut-graphs.json';graphs=4;compiled_shipping='shipping-cut-graphs.json';actual_round_helper_bytes=11;scope='Actual original/current compiled interpolation expressions underPC53, arbitrary symbolic inputs, individual add/mul commutation only; no reassociation'};before=Read "$out/before/validation.json"|Select-Object passed,native;baseline_rebuilt=$baseline.result;after=$native.native;negative_controls='controls.json';source_checks=$checks;production_link='production-linked.json';native_link='native-linked.json';iteration_refresh=@{proof='iteration-refresh/continuations.json';nonvacuous_unsat_obligations=$iteration.obligations.Count;controls='iteration-refresh/controls.json';candidate_used=$iteration.candidate_read;classification='Existing scoped source-iteration Close refreshed for the corrected current real TU'};limits=$limits;game_executed=$false;renderer_executed=$false;python_integration_tests_authored=$false;agents_used=$false;goal_complete=$false;next='Complete destination/contact branch composition and remaining numerical graphs; do not promote whole owners from this slice'}
[IO.File]::WriteAllText("$out/validation.json",($validation|ConvertTo-Json -Depth 25)+"`n")
$ledgerPath="$root/META/function-status.json";$text=[IO.File]::ReadAllText($ledgerPath);$ledger=$text|ConvertFrom-Json
if($text.Contains('"collision-narrowphase-0044cc20-spills-20261029"')){throw 'Preserve immutable audit variants; already recorded'}
$records=@()
foreach($address in @('0044c200','0044cc20','0044d500','0044b2e0','0044d2f0')){
 $prior=@($ledger.records|Where-Object{$_.original_va-eq$address-and$_.implementation-eq'func'})[0]
 $isIteration=$address-eq'0044d2f0'
 $path=if($address-in@('0044c200','0044d2f0')){'src/bh_collision_query.c'}else{'src/bh_collision_model.c'}
 $records+=[ordered]@{id="collision-narrowphase-$address-spills-20261029";original_va=$address;function=$prior.function;role=$prior.role;provenance_confidence=$prior.provenance_confidence;implementation='func';integration='linked_game';source=$path;source_sha256=$hashes[$path];target='build/game/BHunter-source.exe';supersedes=$prior.id;validation=@{classification=$(if($isIteration){'Close (existing scoped production iteration proof refreshed)'}else{'Functional; current original-backed spill correction and real-TU checks, whole-owner proof gaps retained'});close_scoped=$isIteration;raw_byte_match=$false;evidence='META/m4/playable/collision-narrowphase/validation.json';limits=$limits;correction_scope=$(if($address-eq'0044b2e0'){'Loader source body unchanged; fresh actual owning TU/decoded collision consumers/source fixtures'}elseif($isIteration){'175 SMT obligations and mutation controls pass on current object; destination-contact contract remains separate'}else{'Explicit inline/standalone second-cut product spill variants; independent original bodies, compiled graphs and native comparisons'} )};evidence=@('META/m4/playable/collision-narrowphase/validation.json','META/m4/playable/collision-narrowphase/production-linked.json')+@($prior.evidence)}
}
$records+=@{id='collision-narrowphase-original-adapter-20261029';implementation='adapter';integration='linked_harness';role='engine_candidate';source=@('tests/collision_full_original.c','tools/check_collision_full_original.ps1');target=$native.target;game_admitted=$false;validation=@{original_code_executed=$true;production_uses_original_code=$false;scope='Three isolated complete original owners391B/2591B/1738B with verified globals/constants/call bindings; no game loader or uninstalled candidate';evidence='META/m4/playable/collision-narrowphase/final-native/validation.json'}}
$fragment=($records|ConvertTo-Json -Depth 25).Trim();[IO.File]::WriteAllText("$out/ledger-fragment.json",$fragment+"`n")
$anchor='"records": [';$at=$text.IndexOf($anchor);if($at-lt0){throw 'Ledger anchor missing'}
$updated=$text.Insert($at+$anchor.Length,"`n"+$fragment.Substring(1,$fragment.Length-2).Trim()+",");$null=$updated|ConvertFrom-Json
[IO.File]::WriteAllText($ledgerPath,$updated)
Write-Output 'Original-backed two-variant narrow-phase correction admitted; broader collision goal remains active.'
