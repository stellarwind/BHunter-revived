# Static instruction mutations only. Mutated objects are never linked/executed.
param([string]$OutputDirectory='build/collision-destination-controls',
      [string]$EvidenceDirectory='META/m4/playable/collision-destination/scheduling')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out=Join-Path $root $OutputDirectory
$evidence=Join-Path $root $EvidenceDirectory
New-Item -ItemType Directory -Force $out,$evidence|Out-Null
$original=[IO.File]::ReadAllBytes("$root/build/game/bh_collision_query.obj")
$symbolsAt=[BitConverter]::ToUInt32($original,8);$symbolCount=[BitConverter]::ToUInt32($original,12)
$stringsAt=$symbolsAt+18*$symbolCount
$section=0
for($i=0;$i-lt$symbolCount;){
 $at=$symbolsAt+18*$i
 if([BitConverter]::ToUInt32($original,$at)-eq0){
  $start=$stringsAt+[BitConverter]::ToUInt32($original,$at+4);$end=$start
  while($original[$end]){++$end}
  $name=[Text.Encoding]::ASCII.GetString($original,$start,$end-$start)
 }else{$name=[Text.Encoding]::ASCII.GetString($original,$at,8).TrimEnd([char]0)}
 if($name-eq'_destination_tree'){$section=[BitConverter]::ToInt16($original,$at+12)}
 $i+=1+$original[$at+17]
}
if(!$section){throw 'Missing actual production driver'}
$header=20+40*($section-1);$disk=[BitConverter]::ToUInt32($original,$header+20)
$records=@()
foreach($m in @(@('back-child-selection',0xa8,0x34,0x30),@('initial-node',0x13,0x1c,0x18),@('miss-return',0xd2,0xc0,0xc9))){
 $label,$offset,$before,$after=$m
 if($original[$disk+$offset]-ne$before){throw 'Mutation expected instruction changed'}
 $copy=[byte[]]$original.Clone();$copy[$disk+$offset]=$after
 $path="$out/$label.obj";[IO.File]::WriteAllBytes($path,$copy)
 $result=& python "$root/tools/prove_collision_destination.py" --object $path --output "$out/$label" 2>&1
 $status=$LASTEXITCODE;$result|Set-Content "$out/$label.log"
 if($status-eq0-or($result-join"`n")-notmatch'AssertionError: .*sat'){throw "Expected solver counterexample missing: $label"}
 $records+=@{mutation=$label;offset=$offset;before=$before;after=$after;rejected=$true;reason='SMT counterexample';object_sha256=(Get-FileHash $path).Hash.ToLower();log="$OutputDirectory/$label.log"}
 Write-Output "$label rejected by a solver counterexample"
}
@{passed=$true;static_only=$true;mutated_code_executed=$false;controls=$records}|ConvertTo-Json -Depth 8|Set-Content "$evidence/controls.json"
