[CmdletBinding()]
param(
    [ValidateSet('Release', 'Debug')]
    [string]$Configuration = 'Release'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$asiPath = Join-Path $projectRoot "bin\GTA-SA\$Configuration\PCKeyIcons.SA.asi"
# Each variant folder installs the same ASI with its textures as models\pcbtns.txd.
$variants = [ordered]@{
    'Default' = Join-Path $projectRoot 'game_assets\models\pcbtns.txd'
    'Alternate Black' = Join-Path $projectRoot 'game_assets\models\pcbtns-black-alternative.txd'
}

foreach ($inputPath in @($asiPath) + @($variants.Values)) {
    if (-not (Test-Path -LiteralPath $inputPath -PathType Leaf)) {
        throw "Missing package input: $inputPath. Build $Configuration before packaging."
    }
}

$outputDirectory = Join-Path $projectRoot 'bin\packages'
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
$outputDirectory = (Resolve-Path -LiteralPath $outputDirectory).ProviderPath
$archiveName = if ($Configuration -eq 'Release') {
    'PCKeyIcons.zip'
} else {
    'PCKeyIcons-Debug.zip'
}
$archivePath = Join-Path $outputDirectory $archiveName
$stagingName = '.staging-' + [Guid]::NewGuid().ToString('N')
$stagingDirectory = Join-Path $outputDirectory $stagingName
New-Item -ItemType Directory -Path $stagingDirectory | Out-Null

try {
    # Windows PowerShell's archive helpers write '\' entry separators, which
    # other extractors keep in file names. Name each entry with '/' instead.
    Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem
    $stagedArchive = Join-Path $stagingDirectory $archiveName
    $archive = [System.IO.Compression.ZipFile]::Open($stagedArchive, 'Create')
    try {
        foreach ($variant in $variants.GetEnumerator()) {
            $modDirectory = "$($variant.Key)/modloader/PCKeyIcons"
            $entries = @(
                @($asiPath, "$modDirectory/PCKeyIcons.SA.asi"),
                @($variant.Value, "$modDirectory/models/pcbtns.txd")
            )
            foreach ($entry in $entries) {
                [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
                    $archive, $entry[0], $entry[1], 'Optimal') | Out-Null
            }
        }
    } finally {
        $archive.Dispose()
    }
    Copy-Item -LiteralPath $stagedArchive -Destination $archivePath -Force
} finally {
    $resolvedStaging = (Resolve-Path -LiteralPath $stagingDirectory).ProviderPath
    if ((Split-Path -Parent $resolvedStaging) -ne $outputDirectory -or
        (Split-Path -Leaf $resolvedStaging) -ne $stagingName) {
        throw "Refusing to clean an unexpected staging directory: $resolvedStaging"
    }
    Remove-Item -LiteralPath $resolvedStaging -Recurse -Force
}

Write-Output $archivePath
