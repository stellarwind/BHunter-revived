# Record this production variant without modifying earlier candidate evidence.
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out="$root/META/m4/playable/collision-destination"
function Hash([string]$path){return (Get-FileHash $path).Hash.ToLower()}
function Read([string]$path){return Get-Content $path -Raw|ConvertFrom-Json}
$before=Read "$out/plane-before/validation.json";$after=Read "$out/plane-after/validation.json"
$beforeGraph=Read "$out/plane-before/graphs.json";$afterGraph=Read "$out/shipping-plane-graphs.json"
$native=Read "$out/full-native/validation.json";$driver=Read "$out/scheduling/destination.json"
$iteration=Read "$out/iteration-refresh/continuations.json";$replay=Read "$out/smt-replay.json"
$dc=Read "$out/scheduling/controls.json";$ic=Read "$out/iteration-refresh/controls.json"
$positive=Read "$out/exact-positive-control.json";$cuts=Read "$out/shipping-cut-graphs.json"
$shipping=Read "$out/production-linked.json";$nativeLink=Read "$out/native-linked.json"
$game=Read "$root/build/game/build.json";$source=Read "$root/build/collision-destination-source/validation.json"
if($before.passed-or$before.native.plane_differences-ne1-or!$beforeGraph.passed-or$beforeGraph.expressions_match){throw 'Independent before discrepancy required'}
foreach($e in @($after,$afterGraph,$native,$driver,$iteration,$replay,$dc,$ic,$cuts,$game,$source)){if(!$e.passed){throw 'Passing current evidence required'}}
if(!$positive.matched-or$after.native.plane_differences-or$native.native.differences-or$native.native.cached_differences-or$native.native.state_differences){throw 'Current comparison failure'}
$objHash=Hash "$root/build/game/bh_collision_query.obj"
if($driver.object_sha256-ne$objHash-or$iteration.object_sha256-ne$objHash-or$afterGraph.source_object_sha256-ne$objHash-or!$iteration.native_code_equivalence.passed){throw 'Stale production graph/proof variant'}
if($driver.candidate_read-or$iteration.candidate_read){throw 'Candidate must remain separate'}
if($dc.controls.Count-ne3-or$ic.controls.Count-ne4-or$replay.obligations-ne219){throw 'Missing counterexample/replay evidence'}
if($game.executable_sha256-ne(Hash "$root/build/game/BHunter-source.exe")-or$shipping.executable_sha256-ne$game.executable_sha256){throw 'Stale shipping link'}
if($nativeLink.executable_sha256-ne$native.executable_sha256-or$native.executable_sha256-ne(Hash "$root/$($native.target)")){throw 'Stale original-backed native link'}
$reference=Hash "$root/GAME_DATA/BHUNTER.EXE"
if($reference-ne'c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'){throw 'Original changed'}
foreach($item in @(@($driver,"$out/scheduling"),@($iteration,"$out/iteration-refresh"))){
 foreach($p in $item[0].obligations){if($p.result-ne'unsat'-or!$p.nonvacuous-or(Hash "$($item[1])/$($p.smt_file)")-ne$p.smt_sha256){throw 'Stale SMT query'}}
}
$checks=@(Get-Content "$root/build/collision-destination-source/result.log"|Where-Object{$_.StartsWith('{')}|ForEach-Object{$_|ConvertFrom-Json})
if($checks.Count-ne14-or@($checks|Where-Object{$_.failures}).Count){throw 'Connected fixture failure'}
$paths=@('src/bh_collision_query.c','src/bh_collision_query.h','src/bh_collision_model.c','src/bh_collision_model.h',
 'src/bh_collision_core_model.h','src/bh_geometry.c','src/bh_math.c','src/bh_round.c','src/bh_fp.c',
 'tests/collision_full_original.c','tests/collision_plane_original.c','tools/check_collision_full_original.ps1',
 'tools/compare_collision_plane_graph.py','tools/prove_collision_destination.py','tools/check_collision_destination_controls.ps1',
 'tools/prove_collision_iteration.py','tools/check_collision_iteration_controls.ps1','tools/record_collision_destination.ps1')
$hashes=[ordered]@{}
New-Item -ItemType Directory -Force "$out/sources","$out/objects","$out/control-logs"|Out-Null
foreach($path in $paths){
 $hash=Hash "$root/$path";$hashes[$path]=$hash
 Copy-Item -LiteralPath "$root/$path" -Destination "$out/sources/$hash-$([IO.Path]::GetFileName($path)).txt"
 if($native.sources.$path-and$native.sources.$path-ne$hash){throw "Native source changed: $path"}
 if($source.sources.$path-and$source.sources.$path-ne$hash){throw "Connected source changed: $path"}
}
foreach($path in @('build/game/bh_collision_query.obj','build/game/bh_collision_model.obj','build/collision-full-original/bh_collision_query.obj')){
 Copy-Item -LiteralPath "$root/$path" -Destination "$out/objects/$(Hash "$root/$path")-$([IO.Path]::GetFileName($path))"
}
foreach($c in @($dc.controls)+@($ic.controls)){
 Copy-Item -LiteralPath "$root/$($c.log)" -Destination "$out/control-logs/$($c.mutation).log"
}
$limits=@('0044c200 remains Functional at whole-owner level: nested source/destination face-list composition and full polygon/interval numeric graphs remain open',
 'Destination scheduling Close is conditional on corresponding memory-preserving sphere/contact dependency responses and valid normalized logical views, finite acyclic topology, capacity N+1 and bounded work',
 'Plane graph proves selected float32 products/sums at finite PC53 nearest masked precision; matrix dependencies, PC24/64 transitions, exceptional values and FP flags remain separate',
 'Isolated original-backed native targets share actual source math; four cancellation inputs isolate pruning, broader native comparisons are finite diagnostic evidence',
 'No uninstalled candidate read/admission, Python integration-test authoring, agents, game/menu/renderer execution or original modification')
$validation=[ordered]@{date='2026-10-29';passed=$true;state='production_destination_scheduling_scoped_close_and_plane_grouping_correction';family_counts=@{owners=18;exact=3;close_scoped=2;functional_proof_open=13};whole_owner_promotion=$false;production_sources_changed=@('src/bh_collision_query.c');sources=$hashes;original_exe_sha256=$reference;shipping_executable_sha256=$game.executable_sha256;production_translation_units=$game.translation_units;before=@{plane=$before.native;graph='plane-before/graphs.json';retained_object='plane-before/bh_collision_query.obj';retained_source='plane-before/bh_collision_query.c.txt'};after=@{plane=$after.native;native=$native.native};correction='0044c301..0044c33d adds Y/Z products, then X, then plane; old production X/Y-first grouping caused a cancellation-dependent subtree prune';plane_graph='shipping-plane-graphs.json';destination_proof=@{file='scheduling/destination.json';obligations=$driver.obligations.Count;classification='Scoped scheduling Close; whole0044c200 remains Functional';negative_controls='scheduling/controls.json'};source_iteration=@{file='iteration-refresh/continuations.json';obligations=$iteration.obligations.Count;classification='Existing production0044d2f0 scoped Close refreshed';negative_controls='iteration-refresh/controls.json'};smt_replay='smt-replay.json';linked_checks=@('production-linked.json','native-linked.json');source_checks=$checks;byte_match=@{positive_control='exact-positive-control.json';bytes=87;new_match=$false;decision='Current iterative explicit-query ABI and owning layout differ from original global recursive ABI; no easy additional match. Existing87B production vertex positive control passes.'};uninstalled_candidate=@{read=$false;installed=$false;evidence_transferred=$false};limits=$limits;goal_complete=$false;next='Close remaining node_contacts face-list and full narrow-phase numerical composition before promoting whole destination owner'}
[IO.File]::WriteAllText("$out/validation.json",($validation|ConvertTo-Json -Depth 25)+"`n")
$ledgerPath="$root/META/function-status.json";$text=[IO.File]::ReadAllText($ledgerPath);$ledger=$text|ConvertFrom-Json
if($text.Contains('"collision-destination-0044c200-20261029"')){throw 'Variant already recorded'}
$records=@()
foreach($address in @('0044c200','0044d2f0')){
 $prior=@($ledger.records|Where-Object{$_.original_va-eq$address-and$_.implementation-eq'func'})[0]
 $iterationOwner=$address-eq'0044d2f0'
 $records+=[ordered]@{id="collision-destination-$address-20261029";original_va=$address;function=$prior.function;role=$prior.role;provenance_confidence=$prior.provenance_confidence;implementation='func';integration='linked_game';source='src/bh_collision_query.c';source_sha256=$hashes['src/bh_collision_query.c'];target='build/game/BHunter-source.exe';supersedes=$prior.id;validation=@{classification=$(if($iterationOwner){'Close (scoped production iteration proof refreshed)'}else{'Functional whole owner; destination scheduling refinement Close under explicit contact dependencies'});close_scoped=$iterationOwner;component_close=$(if($iterationOwner){'Production source scheduling/suppression/publication'}else{'Production destination work-stack scheduling only'});raw_byte_match=$false;evidence='META/m4/playable/collision-destination/validation.json';limits=$limits};evidence=@('META/m4/playable/collision-destination/validation.json')+@($prior.evidence)}
}
$records+=@{id='collision-destination-plane-original-adapter-20261029';implementation='adapter';integration='linked_harness';role='engine_candidate';source=@('tests/collision_plane_original.c','tests/collision_full_original.c','tools/check_collision_full_original.ps1');target=$after.target;game_admitted=$false;validation=@{original_code_executed=$true;production_uses_original_code=$false;scope='Isolated original plane predicate with only two recursive edges rebound to a counting leaf; logical model fields and source matrix dependencies explicit';evidence='META/m4/playable/collision-destination/plane-after/validation.json'}}
$fragment=($records|ConvertTo-Json -Depth 25).Trim();[IO.File]::WriteAllText("$out/ledger-fragment.json",$fragment+"`n")
$anchor='"records": [';$at=$text.IndexOf($anchor);if($at-lt0){throw 'Ledger anchor missing'}
$updated=$text.Insert($at+$anchor.Length,"`n"+$fragment.Substring(1,$fragment.Length-2).Trim()+",");$null=$updated|ConvertFrom-Json
[IO.File]::WriteAllText($ledgerPath,$updated)
Write-Output 'Current production iteration/destination scheduling evidence recorded; whole destination/contact proof gaps retained.'
