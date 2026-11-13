param([string]$Zig = '', [string]$ConfigInclude = '')
$ErrorActionPreference = 'Stop'
$testRoot = $PSScriptRoot
if (!$Zig) {
    $bundled = Join-Path $testRoot 'bin\python_deps\ziglang\zig.exe'
    if (Test-Path $bundled) { $Zig = $bundled }
    else { $Zig = (Get-Command zig -ErrorAction Stop).Source }
}
if (!$ConfigInclude) { $ConfigInclude = Join-Path $testRoot '..\Templates' }
$out = Join-Path $testRoot 'bin'
New-Item -ItemType Directory -Path $out -Force | Out-Null
$source = Get-Content -LiteralPath (Join-Path $testRoot '..\Src\usbh_bluetooth.c') -Raw
$functions = foreach ($name in @('BT_HCI_QueueCommand', 'BT_ProcessHCICommand', 'BT_QueuePairReply', 'BT_ProcessPairing')) {
    $pattern = '(?m)^static [^;\r\n]+\b' + $name + '\([^;]*?\)\s*\{'
    $match = [regex]::Match($source, $pattern)
    if (!$match.Success) { throw "Missing production function: $name" }
    $start = $source.IndexOf('{', $match.Index)
    $depth = 1
    $end = $start + 1
    while ($depth -gt 0) {
        if ($source[$end] -eq '{') { $depth++ }
        if ($source[$end] -eq '}') { $depth-- }
        $end++
    }
    $source.Substring($match.Index, $end - $match.Index)
}
Set-Content -LiteralPath (Join-Path $out 'hci_functions.inc') -Value ($functions -join "`r`n`r`n") -Encoding ascii
foreach ($test in @('spp_protocol_test', 'acl_transport_test', 'bond_storage_test', 'hci_scheduler_test', 'library_api_test', 'bond_port_failure_test')) {
    $exe = Join-Path $out "$test.exe"
    & $Zig cc -std=c99 -Wall -Wextra -Werror -Wno-sign-compare -Wno-int-to-pointer-cast -DBT_USE_USER_CONFIG -I (Join-Path $testRoot '..\Inc') -I (Join-Path $testRoot 'stubs') -I $ConfigInclude (Join-Path $testRoot "$test.c") -o $exe
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $test" }
    & $exe
    if ($LASTEXITCODE -ne 0) { throw "Test failed: $test" }
}
