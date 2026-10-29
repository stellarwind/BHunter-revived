param(
    [Parameter(Mandatory=$true)][string[]]$Comparison,
    [string]$Executable = 'build/game/BHunter-source.exe',
    [string]$Map = 'build/game/BHunter-source.map',
    [Parameter(Mandatory=$true)][string]$Output
)
# Static inspection only. Normalize verified operands, never differing instructions.
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$reference = [IO.File]::ReadAllBytes((Join-Path $root 'GAME_DATA/BHUNTER.EXE'))
if ((Get-FileHash (Join-Path $root 'GAME_DATA/BHUNTER.EXE')).Hash -ne 'C86205E299119A66E27F687679EE7E978F26FA9A6C0BFB21F664AA6E74E848B1') { throw 'Reference differs' }
$linked = [IO.File]::ReadAllBytes((Join-Path $root $Executable))
$mapText = [IO.File]::ReadAllText((Join-Path $root $Map))
function Read-Image([byte[]]$Image, [long]$Address, [int]$Count) {
    $pe = [BitConverter]::ToInt32($Image,0x3c)
    $optional = $pe + 24
    $imageBase = [BitConverter]::ToUInt32($Image,$optional+28)
    $sectionAt = $optional + [BitConverter]::ToUInt16($Image,$pe+20)
    $sections = [BitConverter]::ToUInt16($Image,$pe+6)
    for ($i=0; $i -lt $sections; ++$i) {
        $at = $sectionAt + 40*$i
        $rva = [BitConverter]::ToUInt32($Image,$at+12)
        $size = [BitConverter]::ToUInt32($Image,$at+16)
        $disk = [BitConverter]::ToUInt32($Image,$at+20)
        $offset = $Address - $imageBase - $rva
        if ($offset -ge 0 -and $offset+$Count -le $size) {
            return ,([byte[]]$Image[($disk+$offset)..($disk+$offset+$Count-1)])
        }
    }
    throw ('Unmapped file bytes at {0:x}' -f $Address)
}
function Symbol-Address([string]$Name) {
    $pattern = '(?m)^\s+[0-9a-fA-F]+:[0-9a-fA-F]+\s+'+[regex]::Escape($Name)+'\s+([0-9a-fA-F]{8})\s'
    $found = [regex]::Matches($mapText,$pattern)
    if ($found.Count -ne 1) { throw "Non-unique linked symbol: $Name" }
    return [Convert]::ToInt64($found[0].Groups[1].Value,16)
}
function Digest([byte[]]$Bytes) {
    return [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($Bytes)).ToLower()
}
$results = @()
foreach ($path in $Comparison) {
    $proof = Get-Content (Join-Path $root $path) -Raw | ConvertFrom-Json
    if ((Get-FileHash (Join-Path $root $proof.object)).Hash.ToLower() -ne $proof.object_sha256) { throw 'Compared object changed' }
    $originalAddress = [Convert]::ToInt64($proof.original_va.Substring(2),16)
    $original = Read-Image $reference $originalAddress $proof.original_length
    if ([Convert]::ToHexString($original).ToLower() -ne $proof.original_hex) { throw 'Compared original bytes differ' }
    $address = Symbol-Address $proof.symbol
    $body = Read-Image $linked $address $proof.original_length
    $rawHash = Digest $body
    $bindings = @()
    foreach ($relocation in $proof.relocations) {
        if (!$relocation.verified) { throw 'Unverified object relocation' }
        $offset = [int]$relocation.offset
        if ($relocation.kind -eq 20) {
            if ($body[$offset-1] -ne 0xe8) { throw 'Expected CALL operand' }
            $target = $address+$offset+4+[BitConverter]::ToInt32($body,$offset)
            if ($target -ne (Symbol-Address $relocation.symbol)) { throw 'Wrong linked callee' }
            $bindings += @{offset=$offset; kind='verified real callee'; symbol=$relocation.symbol; linked_target=('0x{0:x}' -f $target)}
        } elseif ($relocation.kind -eq 6 -and $relocation.constant_hex) {
            if ($body[$offset-2] -notin @(0xd8,0xd9,0xdc,0xdd) -or ($body[$offset-1] -band 0xc7) -ne 5) { throw 'Expected x87 absolute operand' }
            $target = [BitConverter]::ToUInt32($body,$offset)
            $constant = Read-Image $linked $target ($relocation.constant_hex.Length/2)
            if ([Convert]::ToHexString($constant).ToLower() -ne $relocation.constant_hex) { throw 'Wrong linked constant' }
            $bindings += @{offset=$offset;kind='verified initialized constant';linked_target=('0x{0:x}' -f $target);constant_hex=$relocation.constant_hex}
        } else { throw 'Unsupported linked relocation; add an explicit verifier' }
        [Array]::Copy($original,$offset,$body,$offset,4)
    }
    $generated = [Convert]::FromHexString($proof.generated_hex)
    if ($generated.Length -lt $proof.original_length -or $original[-1] -ne 0xc3 -or $generated[$proof.original_length-1] -ne 0xc3) { throw 'Whole RET-terminated owner boundary required' }
    for ($i=$proof.original_length; $i -lt $generated.Length; ++$i) {
        if ($generated[$i] -notin @(0x90,0xcc)) { throw 'Non-padding source tail; prefix comparison rejected' }
    }
    $expected = [byte[]]$generated[0..($proof.original_length-1)]
    if ([Convert]::ToHexString($body) -ne [Convert]::ToHexString($expected)) { throw 'Linked body differs from compared source object' }
    $differences = @(for ($i=0; $i -lt $body.Length; ++$i) { if ($body[$i] -ne $original[$i]) { $i } })
    $results += @{symbol=$proof.symbol;original_va=$proof.original_va;linked_va=('0x{0:x}' -f $address);
        instruction_bytes=$body.Length;linked_matches_compared_object=$true;relocation_normalized_match=($differences.Count -eq 0);
        mismatch_offsets=$differences;bindings=$bindings;raw_body_sha256=$rawHash;normalized_body_sha256=(Digest $body);
        object_sha256=$proof.object_sha256;comparison=$path}
}
$result = @{executable=$Executable;executable_sha256=(Get-FileHash (Join-Path $root $Executable)).Hash.ToLower();
    map=$Map;static_only=$true;bodies=$results}
$result | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $root $Output)
$results | Select-Object symbol,instruction_bytes,linked_matches_compared_object,relocation_normalized_match | Format-Table
