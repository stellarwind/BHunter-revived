# Check complete emitted owner bodies in the private production executable.
param([string]$BuildDirectory='build/physics-source',[switch]$IncludeRotations,
      [string]$EvidenceDirectory='META/m4/playable/collision-audit/integration')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out=[IO.Path]::GetFullPath((Join-Path $root $BuildDirectory))
$evidence=Join-Path $root $EvidenceDirectory
New-Item -ItemType Directory -Force $evidence|Out-Null
$reference="$root/GAME_DATA/BHUNTER.EXE"
if((Get-FileHash $reference).Hash-ne'C86205E299119A66E27F687679EE7E978F26FA9A6C0BFB21F664AA6E74E848B1'){throw 'Reference changed'}
$build=Get-Content "$out/validation.json" -Raw|ConvertFrom-Json
if(!$build.passed){throw 'Passing private production build required'}
$exe="$out/BHunter-physics-source.exe"
if((Get-FileHash $exe).Hash.ToLower()-ne$build.executable_sha256){throw 'Production executable changed'}
$map=Get-Content "$out/BHunter-physics-source.map" -Raw
function ImageBody([string]$path,[long]$address,[int]$length){
    $data=[IO.File]::ReadAllBytes($path)
    $pe=[BitConverter]::ToInt32($data,0x3c)
    if([BitConverter]::ToInt32($data,$pe)-ne0x4550){throw 'Not a PE image'}
    $count=[BitConverter]::ToUInt16($data,$pe+6)
    $base=[BitConverter]::ToUInt32($data,$pe+24+28)
    $first=$pe+24+[BitConverter]::ToUInt16($data,$pe+20)
    $rva=$address-$base
    for($i=0;$i-lt$count;++$i){
        $section=$first+40*$i
        $start=[BitConverter]::ToUInt32($data,$section+12)
        $size=[BitConverter]::ToUInt32($data,$section+16)
        if($rva-ge$start-and$rva+$length-le$start+$size){
            $offset=[BitConverter]::ToUInt32($data,$section+20)+$rva-$start
            return ,([byte[]]$data[$offset..($offset+$length-1)])
        }
    }
    throw 'Owner body is outside raw image sections'
}
$owners=@(
    @('bh_geometry','_bh_geometry_set_flags_core','0x44e210',19,$true),
    @('bh_geometry','_bh_geometry_get_flags_core','0x44e230',15,$true),
    @('bh_geometry','_bh_geometry_vertex_core','0x44e240',87,$true)
)
if($IncludeRotations){$owners+=@(
    @('bh_rotation','_bh_mat34_rotate_world_x','0x401510',251,$false),
    @('bh_rotation','_bh_mat34_rotate_world_y','0x4016e0',247,$false)
)}
$records=@()
foreach($owner in $owners){
    $unit,$symbol,$original,$length,$requireExact=$owner
    $proof="$evidence/$($symbol.TrimStart('_'))-object.json"
    & python "$root/tools/compare_ai_object.py" "$out/objects/$unit.obj" $symbol $original $length --output $proof|Out-Null
    if($LASTEXITCODE){throw 'Static object comparison failed'}
    $comparison=Get-Content $proof -Raw|ConvertFrom-Json
    if($requireExact-and!$comparison.matched){throw "Required exact match differs: $symbol"}
    if($comparison.relocations.Count){throw 'These leaf owners must not contain relocations'}
    $hex=$comparison.generated_hex
    while($hex.EndsWith('90')){$hex=$hex.Substring(0,$hex.Length-2)}
    if(!$hex.EndsWith('c3')){throw 'Complete leaf return not found'}
    $pattern='(?m)^\s+\S+\s+'+[regex]::Escape($symbol)+'\s+([0-9a-fA-F]{8})\s'
    $found=[regex]::Matches($map,$pattern)
    if($found.Count-ne1){throw "Linked owner missing or ambiguous: $symbol"}
    $address=[Convert]::ToInt64($found[0].Groups[1].Value,16)
    $linked=[Convert]::ToHexString((ImageBody $exe $address ($hex.Length/2))).ToLower()
    if($linked-ne$hex){throw "Linked owner differs from emitted production object: $symbol"}
    if($requireExact-and$linked-ne[Convert]::ToHexString((ImageBody $reference ([Convert]::ToInt64($original.Substring(2),16)) $length)).ToLower()){throw 'Linked original comparison failed'}
    $records+=@{symbol=$symbol;original_va=$original;original_length=$length;linked_va=('0x{0:x8}'-f$address);linked_length=$hex.Length/2;linked_hex=$linked;object_matches_linked=$true;raw_original_match=[bool]$comparison.matched;object_proof=$proof}
}
$result=@{passed=$true;target=$exe;executable_sha256=$build.executable_sha256;owners=$records;source_contracts="$out/result.log";sources=$build.sources;limit='Rotation close-match classification requires separate full symbolic/native evidence; object mismatch totals alone establish no close match.'}
[IO.File]::WriteAllText("$evidence/linked.json",($result|ConvertTo-Json -Depth 8))
Write-Output "All $($records.Count) complete owner bodies verified in private production executable."
