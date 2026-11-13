$ErrorActionPreference = 'Stop'
$taskRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskPdsc = Join-Path $taskRoot 'Packaging\MatinKarimi.STM32Bluetooth.pdsc'
[xml]$taskDescription = Get-Content -LiteralPath $taskPdsc -Raw
$taskVersion = $taskDescription.package.releases.release[0].version
if (!$taskVersion) { $taskVersion = $taskDescription.package.releases.release.version }
# Validate against an installed CMSIS schema when available.
$taskSchemaPath = 'C:\Keil_v5\UV4\PACK.xsd'
if (Test-Path -LiteralPath $taskSchemaPath) {
    $taskSchemas = [Xml.Schema.XmlSchemaSet]::new()
    $null = $taskSchemas.Add('', $taskSchemaPath)
    $taskDescription.Schemas = $taskSchemas
    $taskDescription.Validate({ param($sender,$event) throw $event.Message })
}
$taskOutput = Join-Path $taskRoot 'dist'
New-Item -ItemType Directory -Path $taskOutput -Force | Out-Null
$taskPack = Join-Path $taskOutput ("MatinKarimi.STM32Bluetooth.{0}.pack" -f $taskVersion)
foreach ($taskFile in $taskDescription.SelectNodes('//components/component/files/file')) {
    $taskPath = Join-Path $taskRoot $taskFile.name
    if (!(Test-Path -LiteralPath $taskPath)) { throw "Missing component file: $taskPath" }
}
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$taskStream = [IO.File]::Open($taskPack, [IO.FileMode]::Create)
$taskArchive = [IO.Compression.ZipArchive]::new($taskStream, [IO.Compression.ZipArchiveMode]::Create)
try {
    [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($taskArchive, $taskPdsc, 'MatinKarimi.STM32Bluetooth.pdsc') | Out-Null
    foreach ($taskDirectory in @('Inc','Src','Ports','Templates','Examples','Documentation')) {
        Get-ChildItem -LiteralPath (Join-Path $taskRoot $taskDirectory) -File -Recurse | ForEach-Object {
            $taskRelative = $_.FullName.Substring($taskRoot.Length + 1).Replace('\','/')
            [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($taskArchive, $_.FullName, $taskRelative) | Out-Null
        }
    }
    foreach ($taskName in @('LICENSE','README.md')) {
        [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($taskArchive,(Join-Path $taskRoot $taskName),$taskName) | Out-Null
    }
} finally { $taskArchive.Dispose(); $taskStream.Dispose() }
Copy-Item -LiteralPath $taskPdsc -Destination (Join-Path $taskOutput 'MatinKarimi.STM32Bluetooth.pdsc') -Force
$taskChecksums = foreach ($taskArtifact in @($taskPack, (Join-Path $taskOutput 'MatinKarimi.STM32Bluetooth.pdsc'))) {
    $taskHash = Get-FileHash -LiteralPath $taskArtifact -Algorithm SHA256
    '{0}  {1}' -f $taskHash.Hash.ToLowerInvariant(), [IO.Path]::GetFileName($taskArtifact)
}
Set-Content -LiteralPath (Join-Path $taskOutput 'SHA256SUMS') -Value $taskChecksums -Encoding ascii
Write-Output $taskPack
Write-Output $taskChecksums
