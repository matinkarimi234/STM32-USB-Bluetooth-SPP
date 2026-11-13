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
foreach ($test in @('spp_protocol_test', 'acl_transport_test', 'bond_storage_test', 'hci_scheduler_test', 'library_api_test', 'bond_port_failure_test', 'periodic_callback_test')) {
    $exe = Join-Path $out "$test.exe"
    & $Zig cc -std=c99 -Wall -Wextra -Werror -Wno-sign-compare -Wno-int-to-pointer-cast -DBT_USE_USER_CONFIG -I (Join-Path $testRoot '..\Inc') -I (Join-Path $testRoot 'stubs') -I $ConfigInclude -I (Join-Path $testRoot '..\Examples\PeriodicCallback') (Join-Path $testRoot "$test.c") -o $exe
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $test" }
    & $exe
    if ($LASTEXITCODE -ne 0) { throw "Test failed: $test" }
}

# Every supported F746 erase geometry compiles and exercises the concrete port.
$layoutTemplate = Get-Content -LiteralPath (Join-Path $testRoot '..\Templates\bt_user_config.h') -Raw
$sectors = @('0x08000000UL','0x08008000UL','0x08010000UL','0x08018000UL','0x08020000UL','0x08040000UL','0x08080000UL','0x080C0000UL')
foreach ($sector in $sectors) {
    $configDir = Join-Path $out $sector
    New-Item -ItemType Directory -Path $configDir -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $configDir 'bt_user_config.h') -Value ($layoutTemplate.Replace('0x080C0000UL',$sector)) -Encoding ascii
    $exe = Join-Path $out ("bond-$sector.exe")
    & $Zig cc -std=c99 -Wall -Wextra -Werror -Wno-sign-compare -Wno-int-to-pointer-cast -I (Join-Path $testRoot '..\Inc') -I (Join-Path $testRoot 'stubs') -I $configDir (Join-Path $testRoot 'bond_storage_test.c') -o $exe
    if ($LASTEXITCODE -ne 0) { throw "Flash geometry compilation failed: $sector" }
    & $exe
    if ($LASTEXITCODE -ne 0) { throw "Flash geometry test failed: $sector" }
}
Write-Output 'All eight F746 sector layouts passed; physical flash-size bounds were checked.'
