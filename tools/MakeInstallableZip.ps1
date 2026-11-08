param(
    [string]$OutputZip = "Grindstone Sword Fix.zip"
)

$ErrorActionPreference = "Stop"
$root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")).Path
$distData = Join-Path $root "dist\Data"

# The installer carries what the game needs plus the GPL-3.0 license text.
$required = @(
    "SKSE/Plugins/GrindstoneSwordFix.dll"
)

$missing = @()
if (-not (Test-Path -LiteralPath (Join-Path $root "LICENSE.txt"))) {
    $missing += "LICENSE.txt"
}
foreach ($relative in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $distData $relative))) {
        $missing += "dist/Data/$relative"
    }
}
if ($missing.Count -gt 0) {
    throw "Installable package is incomplete. Missing:`n - $($missing -join "`n - ")"
}

$outPath = Join-Path $root $OutputZip
if (Test-Path -LiteralPath $outPath) {
    Remove-Item -LiteralPath $outPath -Force
}

Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [System.IO.Compression.ZipFile]::Open(
    $outPath,
    [System.IO.Compression.ZipArchiveMode]::Create)
try {
    foreach ($relative in $required) {
        [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
            $zip,
            (Join-Path $distData $relative),
            $relative,
            [System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
    [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
        $zip,
        (Join-Path $root "LICENSE.txt"),
        "LICENSE.txt",
        [System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
}
finally {
    $zip.Dispose()
}

Write-Host "Created installable package: $outPath" -ForegroundColor Green
