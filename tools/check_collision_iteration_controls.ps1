# Static COFF mutations only; no mutated code is linked or executed.
param([string]$OutputDirectory='build/collision-iteration-controls',
      [string]$EvidenceDirectory='META/m4/playable/collision-iteration')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out=Join-Path $root $OutputDirectory
$evidence=Join-Path $root $EvidenceDirectory
New-Item -ItemType Directory -Force $out,$evidence|Out-Null
$original=[IO.File]::ReadAllBytes("$root/build/game/bh_collision_query.obj")
$symbolsAt=[BitConverter]::ToUInt32($original,8);$symbolCount=[BitConverter]::ToUInt32($original,12)
$stringsAt=$symbolsAt+18*$symbolCount
function ReadName([int]$at){
 if([BitConverter]::ToUInt32($original,$at)-eq0){$start=$stringsAt+[BitConverter]::ToUInt32($original,$at+4);$end=$start;while($original[$end]){++$end};return [Text.Encoding]::ASCII.GetString($original,$start,$end-$start)}
 return [Text.Encoding]::ASCII.GetString($original,$at,8).TrimEnd([char]0)
}
$sections=@{}
for($i=0;$i-lt$symbolCount;){
 $at=$symbolsAt+18*$i;$name=ReadName $at
 if($name-in@('_execute','_mark_faces')){$sections[$name]=[BitConverter]::ToInt16($original,$at+12)}
 $i+=1+$original[$at+17]
}
$mutations=@(
 @('resume-phase','_execute',0x191,1,2),
 @('front-back-order','_execute',0x1a9,0x30,0x34),
 @('suppression-direction','_mark_faces',0x67,0x41,0x49),
 @('hit-output-publication','_execute',0x255,0x75,0x74)
)
$records=@()
foreach($m in $mutations){
 $label,$owner,$offset,$before,$after=$m
 if(!$sections.ContainsKey($owner)){throw 'Missing production owner'}
 $header=20+40*($sections[$owner]-1);$disk=[BitConverter]::ToUInt32($original,$header+20)
 if($original[$disk+$offset]-ne$before){throw 'Mutation does not identify the expected actual instruction'}
 $copy=[byte[]]$original.Clone();$copy[$disk+$offset]=$after
 $path="$out/$label.obj";[IO.File]::WriteAllBytes($path,$copy)
 $result=& python "$root/tools/prove_collision_iteration.py" --object $path --output "$out/$label" 2>&1
 $status=$LASTEXITCODE;$result|Set-Content "$out/$label.log"
 if($status-eq0-or($result-join"`n")-notmatch'AssertionError: .*sat'){throw "Mutation did not produce a solver counterexample: $label"}
 $records+=@{mutation=$label;owner=$owner;offset=$offset;before=$before;after=$after;rejected=$true;reason='SMT found a counterexample to a required production refinement obligation';object_sha256=(Get-FileHash $path).Hash.ToLower();log="$OutputDirectory/$label.log"}
 Write-Output "$label rejected by a solver counterexample"
}
@{passed=$true;static_only=$true;mutated_code_executed=$false;controls=$records}|ConvertTo-Json -Depth 8|Set-Content "$evidence/controls.json"
