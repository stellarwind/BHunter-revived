param([string]$EvidenceDirectory='META/m4/playable/collision-iteration/native',[switch]$PlaneProbe)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out=if($PlaneProbe){"$root/build/collision-plane-original"}else{"$root/build/collision-full-original"}
$evidence=Join-Path $root $EvidenceDirectory
New-Item -ItemType Directory -Force $out,$evidence|Out-Null
$reference="$root/GAME_DATA/BHUNTER.EXE"
if((Get-FileHash $reference).Hash-ne'C86205E299119A66E27F687679EE7E978F26FA9A6C0BFB21F664AA6E74E848B1'){throw 'Original changed'}
$image=[IO.File]::ReadAllBytes($reference)
function Raw([int]$address,[int]$length){
 $offset=if($address-lt0x489000){$address-0x400c00}else{$address-0x489000+0x87a00}
 return ,([byte[]]$image[$offset..($offset+$length-1)])
}
$owners=@(@(0x44d2f0,391,0),@(0x44c200,2591,512),@(0x44cc20,1738,5120))
$header=@('static void install_original_collision(unsigned char *p){unsigned long address;')
$calls=@();$globals=@();$constants=@()
foreach($owner in $owners){
 $start,$length,$base=$owner
 $body=Raw $start $length
 $header+=('{unsigned char bytes[]={'+($body-join',')+'};memcpy(p+'+$base+',bytes,sizeof(bytes));}')
 $path=('{0:x8}'-f$start)
 $rowsPath=if($start-eq0x44cc20){"$root/META/m4/face-contact/original/disassembly.json"}else{"$root/META/m4/collision-traversal/original/$path.disassembly.json"}
 $rows=Get-Content $rowsPath -Raw|ConvertFrom-Json
 $cursor=$start
 foreach($row in $rows){
  $va=[Convert]::ToInt32($row.address,16);$bytes=[Convert]::FromHexString($row.bytes)
  if($va-ne$cursor-or[Convert]::ToHexString((Raw $va $bytes.Length))-ne$row.bytes.ToUpper()){throw 'Incomplete original body'}
  $cursor+=$bytes.Length;$at=$base+$va-$start
  if($row.mnemonic-eq'CALL'){
   $target=[Convert]::ToInt32($row.operands[0].Substring(2),16)
   $bound=switch($target){0x44d2f0{'p'}0x44c200{'p+512'}0x4011a0{'(unsigned char *)bh_vec3_cross'}0x4012e0{'(unsigned char *)bh_mat34_transform_point'}0x401360{'(unsigned char *)bh_mat34_transform_direction'}0x4013d0{'(unsigned char *)bh_mat34_inverse_transform_point'}default{throw 'Unreviewed callee'}}
   if($bytes[0]-ne0xe8-or$bytes.Length-ne5){throw 'Call encoding'}
   $header+=('address=(unsigned long)('+$bound+')-(unsigned long)(p+'+($at+5)+');memcpy(p+'+($at+1)+',&address,4);')
   $calls+=@{owner=$path;at=$va;target=$target;binding=$bound}
  }elseif($row.mnemonic-match'^J'){
   $target=[Convert]::ToInt32($row.operands[0].Substring(2),16)
   if($target-lt$start-or$target-ge$start+$length){throw 'External branch'}
  }
  foreach($operand in $row.operands){
   if($operand-match'(?:\[)?0x(0*4e[0-9a-f]+|0*489[0-9a-f]+)(?:\])?$'){
    $target=[Convert]::ToInt32($Matches[1],16)
    $pos=$bytes.Length-4
    if($pos-lt1-or[BitConverter]::ToInt32($bytes,$pos)-ne$target){throw 'Absolute operand encoding'}
    if($target-ge0x4e0fc4-and$target-le0x4e104c){
     $offset=$target-0x4e0fc4
     $header+=('address=(unsigned long)(original_state+'+$offset+');memcpy(p+'+($at+$pos)+',&address,4);')
     $globals+=@{owner=$path;at=$va;original=$target;state_offset=$offset}
    }elseif($target-in@(0x489e98,0x489ea0,0x489ea4,0x489ea8,0x489eb0)){
     $width=if($operand.StartsWith('double')){8}else{4}
     $value=Raw $target $width;$slot=4096+$target-0x489e98
     $header+=('{unsigned char bytes[]={'+($value-join',')+'};memcpy(p+'+$slot+',bytes,sizeof(bytes));}')
     $header+=('address=(unsigned long)(p+'+$slot+');memcpy(p+'+($at+$pos)+',&address,4);')
     $constants+=@{owner=$path;at=$va;original=$target;bytes=[Convert]::ToHexString($value)}
    }else{throw 'Unreviewed global/constant'}
   }
  }
 }
 if($cursor-ne$start+$length){throw 'Incomplete owner'}
}
$header+='}';[IO.File]::WriteAllLines("$out/original-full.h",$header)
$vc='D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC';$sp='D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'
$env:PATH="$root/analysis/toolchain-sp3/bin;D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;$env:PATH";$env:INCLUDE="$sp/include;$vc/include";$env:LIB="$sp/lib;$vc/lib"
foreach($name in @('CL','_CL_','LINK','_LINK_')){[Environment]::SetEnvironmentVariable($name,$null,'Process')}
$sources=@($(if($PlaneProbe){'tests/collision_plane_original.c'}else{'tests/collision_full_original.c'}),'src/bh_collision_query.c','src/bh_collision_model.c','src/bh_geometry.c','src/bh_bsp.c','src/bh_vfile.c','src/bh_math.c','src/bh_round.c','src/bh_fp.c','src/bh_collision_cache.c')
$objects=@();$hashes=@{}
foreach($source in $sources){
 $obj="$out/$([IO.Path]::GetFileNameWithoutExtension($source)).obj";$objects+=$obj
 & "$root/analysis/toolchain-sp3/bin/CL.EXE" /nologo /c /ML /O2 /G5 /Oi /Gy "/Fo$obj" "$root/$source"
 if($LASTEXITCODE){throw 'Full original/source real-TU compile failed'}
 $hashes[$source]=(Get-FileHash "$root/$source").Hash.ToLower()
}
$rsp=@('/nologo','/opt:ref','kernel32.lib')+$objects+@("/out:$out/collision-full-original.exe","/map:$out/collision-full-original.map")
[IO.File]::WriteAllLines("$out/link.rsp",@($rsp|ForEach-Object{'"'+$_+'"'}));& "$vc/bin/LINK.EXE" "@$out/link.rsp"
if($LASTEXITCODE){throw 'Full original/source link failed'}
$result=& "$out/collision-full-original.exe";$code=$LASTEXITCODE;$result|Tee-Object "$out/result.log"
@{passed=($code-eq0);plane_probe=[bool]$PlaneProbe;native=($result|Select-Object -Last 1|ConvertFrom-Json);target=$(if($PlaneProbe){'build/collision-plane-original/collision-full-original.exe'}else{'build/collision-full-original/collision-full-original.exe'});executable_sha256=(Get-FileHash "$out/collision-full-original.exe").Hash.ToLower();sources=$hashes;calls=$calls;globals=$globals;constants=$constants;limits=@('Isolated complete391B source,2591B destination traversal and1738B cached-contact bodies; no original image loader or game execution','PlaneProbe rebinds only the two destination recursive edges to a counting leaf, isolating the plane-cull decision; it is not an end-to-end collision comparison','Actual source math is intentionally shared by both paths; dependency equivalence to original math is not established here','Finite PC53 native comparisons are diagnostic, not arbitrary-domain symbolic proofs')}|ConvertTo-Json -Depth 12|Set-Content "$evidence/validation.json"
if($code){throw 'Full original/source differences found'}
