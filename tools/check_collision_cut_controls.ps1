# Static instruction mutations; no mutated object is linked or executed.
param([string]$OutputDirectory='build/collision-cut-controls',
      [string]$EvidenceDirectory='META/m4/playable/collision-narrowphase')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out=Join-Path $root $OutputDirectory;$evidence=Join-Path $root $EvidenceDirectory
New-Item -ItemType Directory -Force $out|Out-Null
$data=[IO.File]::ReadAllBytes("$root/build/collision-full-original/bh_collision_model.obj")
$table=[BitConverter]::ToUInt32($data,8);$count=[BitConverter]::ToUInt32($data,12);$strings=$table+18*$count;$section=0
for($i=0;$i-lt$count;){
 $at=$table+18*$i
 if([BitConverter]::ToUInt32($data,$at)-eq0){$start=$strings+[BitConverter]::ToUInt32($data,$at+4);$end=$start;while($data[$end]){++$end};$name=[Text.Encoding]::ASCII.GetString($data,$start,$end-$start)}
 else{$name=[Text.Encoding]::ASCII.GetString($data,$at,8).TrimEnd([char]0)}
 if($name-eq'_face_cut'){$section=[BitConverter]::ToInt16($data,$at+12)}
 $i+=1+$data[$at+17]
}
if(!$section){throw 'Actual face-cut owner missing'}
$raw=[BitConverter]::ToUInt32($data,20+40*($section-1)+20);$results=@()
foreach($m in @(@('merge-owner-Y-rounding',0x1a0,0x75,0x74),@('wrong-previous-product',0x16d,0x48,0x3c))){
 $label,$offset,$before,$after=$m
 if($data[$raw+$offset]-ne$before){throw 'Mutation instruction context differs'}
 $copy=[byte[]]$data.Clone();$copy[$raw+$offset]=$after;$path="$out/$label.obj";[IO.File]::WriteAllBytes($path,$copy)
 $text=& python "$root/tools/compare_collision_cut_graph.py" --object $path --output "$out/$label.json" 2>&1
 $status=$LASTEXITCODE;$text|Set-Content "$out/$label.log"
 if($status-eq0-or($text-join"`n")-notmatch'Actual compiled source differs'){throw 'Mutation not rejected by original/source expression comparison'}
 $results+=@{name=$label;rejected=$true;offset=$offset;before=$before;after=$after;object_sha256=(Get-FileHash $path).Hash.ToLower()}
}
@{passed=$true;static_only=$true;mutated_code_executed=$false;controls=$results}|ConvertTo-Json -Depth 8|Set-Content "$evidence/controls.json"
Write-Output 'Both original-backed cut graph mutations rejected.'
