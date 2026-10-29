# Reconcile current collision implementations, keeping matching and integration separate.
param([string]$BuildDirectory='build/collision-audit-source')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out=Join-Path $root 'META/m4/playable/collision-audit'
New-Item -ItemType Directory -Force $out|Out-Null
$ledgerPath=Join-Path $root 'META/function-status.json'
$ledgerText=[IO.File]::ReadAllText($ledgerPath)
$ledger=$ledgerText|ConvertFrom-Json
if($ledgerText.Contains('"collision-audit-0044b0a0-20261029"')){throw 'Audit already recorded; preserve its immutable ledger variants'}
$build=Get-Content "$root/$BuildDirectory/validation.json" -Raw|ConvertFrom-Json
$game=Get-Content "$root/build/game/build.json" -Raw|ConvertFrom-Json
$linked=Get-Content "$out/production-linked.json" -Raw|ConvertFrom-Json
$exact=Get-Content "$out/integration/linked.json" -Raw|ConvertFrom-Json
$native=Get-Content "$out/geometry/validation.json" -Raw|ConvertFrom-Json
if(!$build.passed-or!$game.passed-or!$exact.passed-or!$native.passed){throw 'Passing current builds required'}
$referenceHash=(Get-FileHash "$root/GAME_DATA/BHUNTER.EXE").Hash.ToLower()
if($referenceHash-ne'c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'){throw 'Original changed'}
if((Get-FileHash "$root/build/game/BHunter-source.exe").Hash.ToLower()-ne$game.executable_sha256-or$linked.executable_sha256-ne$game.executable_sha256){throw 'Stale production binding audit'}
if((Get-FileHash $build.target).Hash.ToLower()-ne$build.executable_sha256-or$exact.executable_sha256-ne$build.executable_sha256){throw 'Stale private build audit'}
$rows=@(
 @('0044b0a0','collision-resource','Explicit prefix/search ownership and bounded strings; no complete instruction graph proof'),
 @('0044b130','collision-cache','Explicit five-allocation owner, successful-path FP/spill graph and real consumer metadata binding remain open for Close'),
 @('0044b2e0','collision-model','Explicit decoded ownership, file search and rollback; full original allocation/read graph not proved'),
 @('0044b7a0','collision-box','Serialized typed payload and decoded metadata adapter; complete original numerical/write graph not proved'),
 @('0044c200','collision-traversal','Explicit stacks/suppression and shared narrow phase; complete original branch/FP/contact graph not proved'),
 @('0044cc20','collision-model','Face clipping/intersection numerics and shared transform dependencies need complete instruction proof'),
 @('0044d2f0','collision-traversal','Native gate correction retained; standalone recursive candidate remains uninstalled, destination dependency not proved'),
 @('0044d480','physics-response','Explicit overlap context replaces original global publication and lazy cache calls; child graph not proved'),
 @('0044d500','swept-collision','Cached-contact explicit context replaces globals/tailcall; depends on unresolved complete face graph'),
 @('0044d530','segment-query','Retained original native segment evidence is finite/PC53 and shares source math; full graph and runtime FP transitions open'),
 @('0044d9b0','segment-owner','Explicit cache/query lifetime and transforms; complete original wrapper/dependency graph not proved'),
 @('0044da20','swept-collision','Endpoint/three-bisection/fallback checks are functional; near-degenerate numerics, child composition and runtime precision open'),
 @('0044e060','geometry-faces','Source callbacks, owner layout and guards are adapters; complete original descriptor/enumeration graph not proved'),
 @('0044e210','geometry-faces','Exact core only; checked source API and field-view binding are separate adapters'),
 @('0044e230','geometry-faces','Exact core only; checked source API and field-view binding are separate adapters'),
 @('0044e240','geometry-faces','Exact core only; live bounded chain and source view supplied by checked adapter; validation walks the chain before the core'),
 @('0044e2a0','geometry-faces','Descriptor lookup/overwrite and missing-normal contracts checked functionally; no complete instruction graph proof'),
 @('0044e3f0','geometry-radius','Root-corner norm and asymmetric final float spill checked functionally; no complete original FP graph proof')
)
$map=Get-Content "$root/build/game/BHunter-source.map" -Raw
$functionMap=Import-Csv "$root/analysis/function-map.tsv" -Delimiter "`t"
$roles=(Get-Content "$root/META/m1/role-catalog.json" -Raw|ConvertFrom-Json).records
$reference=[IO.File]::ReadAllBytes("$root/GAME_DATA/BHUNTER.EXE")
$scenarios=@(Get-Content "$root/$BuildDirectory/result.log"|Where-Object{$_.StartsWith('{')}|ForEach-Object{$_|ConvertFrom-Json})
if(@($scenarios|Where-Object{$_.failures-ne0}).Count){throw 'Source scenario failure'}
$records=@();$partition=@()
foreach($row in $rows){
 $address,$scenario,$limit=$row
 $prior=@($ledger.records|Where-Object{$_.original_va-eq$address-and$_.implementation-eq'func'})|Select-Object -Last 1
 if(!$prior){throw "Missing functional provenance: $address"}
 $symbol=$prior.function
 if($address-eq'0044d530'){$symbol='bh_segment_query_run'}
 $proof=@($exact.owners|Where-Object{$_.original_va-eq('0x'+$address.Substring(2))})
 if($proof.Count){$symbol=$proof[0].symbol.TrimStart('_')}
 $source=$prior.source
 $hash=(Get-FileHash "$root/$source").Hash.ToLower()
 if($build.sources.$source-ne$hash){throw "Source snapshot differs: $source"}
 if(!@($scenarios|Where-Object{$_.scenario-eq$scenario}).Count){throw "Missing source exercise: $scenario"}
 $bounds=@($roles|Where-Object{$_.address-eq$address})[0].body_ranges
 $catalog=@($functionMap|Where-Object{$_.address-eq$address})[0]
 if($bounds.Count-ne1){throw 'Review noncontiguous boundaries separately'}
 $ends=$bounds[0]-split '-';$start=[Convert]::ToInt32($ends[0],16);$last=[Convert]::ToInt32($ends[1],16)
 $length=$last-$start+1
 if($length-ne[int]$catalog.body_bytes){throw 'Boundary/catalog disagreement'}
 # All selected owners are in the verified original .text section.
 $body=[byte[]]$reference[($start-0x400c00)..($last-0x400c00)]
 $bodyHash=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($body)).ToLower()
 $isExact=$proof.Count-eq1-and$proof[0].raw_original_match
 $public=[regex]::IsMatch($map,'(?m)^\s+\w+:\w+\s+_'+[regex]::Escape($symbol)+'\s+[0-9a-fA-F]{8}\s')
 if($public-and$game.sources.$source-ne$hash){throw 'Game source differs'}
 $implementation=if($isExact){'bm'}else{'func'}
 $target=if($public){'build/game/BHunter-source.exe'}else{"$BuildDirectory/physics-source-checks.exe"}
 $id="collision-audit-$address-20261029"
 $record=[ordered]@{id=$id;original_va=$address;function=$symbol;role=$prior.role;provenance_confidence=$prior.provenance_confidence;source=$source;source_sha256=$hash;implementation=$implementation;integration=$(if($public){'linked_game'}else{'linked_harness'});target=$target;prior_functional_record=$prior.id;validation=[ordered]@{evidence='META/m4/playable/collision-audit/validation.json';boundary=$bounds[0];original_body_sha256=$bodyHash;complete_original_bytes=$length;raw_byte_match=[bool]$isExact;source_scenario=$scenario;limit=$limit;original_code_in_shipping=$false;close_claim=$false};evidence=@('META/m4/playable/collision-audit/production-linked.json','META/m4/playable/collision-audit/integration/linked.json')+@($prior.evidence)}
 if($isExact){
  $record.source=@('src/bh_geometry.c','src/bh_geometry_core.h','src/bh_geometry_core.inc')
  $hashes=[ordered]@{};foreach($path in $record.source){$hashes[$path]=(Get-FileHash "$root/$path").Hash.ToLower()}
  $record.source_sha256=$hashes
  $record.validation['exact_core_symbol']=$proof[0].symbol
  $record.validation['native_evidence']='META/m4/playable/collision-audit/geometry/validation.json'
 }
 $records+=$record
 $partition+=@{address=$address;implementation=$implementation;function=$symbol;integration=$record.integration;limit=$limit;prior_evidence=$prior.evidence}
}
$result=[ordered]@{date='2026-10-29';scope='18 original collision resource/model/cache/box/query/segment/sweep/geometry owners 0044b0a0..0044e3f0; adjacent physics/AI callers are dependencies, not extra collision owners';review_kind='Existing per-owner contract/evidence reconciliation plus fresh current real-TU integration; new complete byte proof for vertex, refreshed exact flag cores';passed=$true;counts=@{owners=18;exact=3;functional=15;close_scoped=0};owners=$partition;original_exe_sha256=$referenceHash;production_executable_sha256=$game.executable_sha256;production_translation_units=$game.translation_units;native=$native.native;source_scenarios=$scenarios;source_build="$BuildDirectory/validation.json";game_executed=$false;python_authored=$false;limits=@('Functional means the declared source contracts; no whole-family 1:1 or whole-owner Close claim inferred from native/fixture totals','Cache and segment-owner are linked/exercised in the native source target but absent from the shipping map; no linked-game claim for those variants','Historical evidence remains attached to its original source variant; fresh evidence uses current hashes','M4, runtime PC24 transitions, degenerate contacts and full world/actor collision composition remain unresolved')}
[IO.File]::WriteAllText("$out/validation.json",($result|ConvertTo-Json -Depth 18)+"`n")
[IO.File]::WriteAllText("$out/ledger-fragment.json",($records|ConvertTo-Json -Depth 18)+"`n")
# Prepend this audit without reformatting or rewriting earlier evidence.
$fragment=($records|ConvertTo-Json -Depth 18).Trim()
$contents=$fragment.Substring(1,$fragment.Length-2).Trim()
$anchor='"records": ['
$at=$ledgerText.IndexOf($anchor)
if($at-lt0){throw 'Ledger records anchor missing'}
$at+=$anchor.Length
$updated=$ledgerText.Insert($at,"`n"+$contents+",")
$null=$updated|ConvertFrom-Json
[IO.File]::WriteAllText($ledgerPath,$updated)
Write-Output '18 collision owners reconciled: 3 exact cores, 15 Functional with explicit remaining proof limits.'
