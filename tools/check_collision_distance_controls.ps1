# Static COFF controls; never link or execute mutated code.
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out="$root/build/collision-distance-controls";$evidence="$root/META/m4/playable/collision-plane-evaluation"
New-Item -ItemType Directory -Force $out|Out-Null
$data=[IO.File]::ReadAllBytes("$root/build/game/bh_collision_model.obj")
$table=[BitConverter]::ToUInt32($data,8);$count=[BitConverter]::ToUInt32($data,12);$strings=$table+18*$count
$sections=@{}
for($i=0;$i-lt$count;){
 $at=$table+18*$i
 if([BitConverter]::ToUInt32($data,$at)-eq0){$start=$strings+[BitConverter]::ToUInt32($data,$at+4);$end=$start;while($data[$end]){++$end};$name=[Text.Encoding]::ASCII.GetString($data,$start,$end-$start)}
 else{$name=[Text.Encoding]::ASCII.GetString($data,$at,8).TrimEnd([char]0)}
 if($name-in@('_cut_distance','_face_cut')){$sections[$name]=[BitConverter]::ToInt16($data,$at+12)}
 $i+=1+$data[$at+17]
}
$results=@()
foreach($m in @(@('wrong-owner-grouping','_cut_distance',0xe,0x74,0x75,'Actual compiled plane expressions differ'),
                @('wrong-owner-forwarding','_face_cut',0x117,0x24,0x1c,'AssertionError: .*sat'))){
 $label,$owner,$offset,$before,$after,$reason=$m
 if(!$sections[$owner]){throw 'Missing actual production owner'}
 $raw=[BitConverter]::ToUInt32($data,20+40*($sections[$owner]-1)+20)
 if($data[$raw+$offset]-ne$before){throw 'Mutation context changed'}
 $copy=[byte[]]$data.Clone();$copy[$raw+$offset]=$after;$path="$out/$label.obj";[IO.File]::WriteAllBytes($path,$copy)
 $text=& python "$root/tools/compare_collision_distance_graph.py" --object $path --round-object "$root/build/game/bh_round.obj" --output "$out/$label.json" 2>&1
 $status=$LASTEXITCODE;$text|Set-Content "$out/$label.log"
 if($status-eq0-or($text-join"`n")-notmatch$reason){throw "Mutation not independently rejected: $label"}
 $results+=@{name=$label;owner=$owner;rejected=$true;offset=$offset;before=$before;after=$after;reason=$reason;object_sha256=(Get-FileHash $path).Hash.ToLower();log="build/collision-distance-controls/$label.log"}
 Write-Output "$label rejected"
}
@{passed=$true;static_only=$true;mutated_code_executed=$false;controls=$results}|ConvertTo-Json -Depth 8|Set-Content "$evidence/controls.json"
