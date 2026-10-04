[CmdletBinding()]
param(
    [ValidateSet('Release', 'Debug')]
    [string]$Configuration = 'Release'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$asiPath = Join-Path $projectRoot "bin\GTA-SA\$Configuration\PCKeyIcons.SA.asi"
$texturePath = Join-Path $projectRoot 'game_assets\models\pcbtns.txd'

foreach ($inputPath in @($asiPath, $texturePath)) {
    if (-not (Test-Path -LiteralPath $inputPath -PathType Leaf)) {
        throw "Missing package input: $inputPath. Build $Configuration before packaging."
    }
}

$outputDirectory = Join-Path $projectRoot 'bin\packages'
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
$outputDirectory = (Resolve-Path -LiteralPath $outputDirectory).ProviderPath
$archiveName = if ($Configuration -eq 'Release') {
    'PCKeyIcons-ModLoader.zip'
} else {
    'PCKeyIcons-ModLoader-Debug.zip'
}
$archivePath = Join-Path $outputDirectory $archiveName
$stagingName = '.staging-' + [Guid]::NewGuid().ToString('N')
$stagingDirectory = Join-Path $outputDirectory $stagingName
New-Item -ItemType Directory -Path $stagingDirectory | Out-Null

try {
    $modDirectory = Join-Path $stagingDirectory 'modloader\PCKeyIcons'
    $modelsDirectory = Join-Path $modDirectory 'models'
    New-Item -ItemType Directory -Path $modelsDirectory -Force | Out-Null
    Copy-Item -LiteralPath $asiPath -Destination (Join-Path $modDirectory 'PCKeyIcons.SA.asi')
    Copy-Item -LiteralPath $texturePath -Destination (Join-Path $modelsDirectory 'pcbtns.txd')

    $stagedArchive = Join-Path $stagingDirectory $archiveName
    Compress-Archive -LiteralPath (Join-Path $stagingDirectory 'modloader') `
        -DestinationPath $stagedArchive -CompressionLevel Optimal
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
