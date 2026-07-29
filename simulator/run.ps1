# Compile (si besoin) puis lance le simulateur PC.
# Usage : .\simulator\run.ps1 [arguments passes a l'executable]

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

& "$scriptDir\build.ps1"
if ($LASTEXITCODE -ne 0) { exit 1 }

$exe = Join-Path $scriptDir "build\trophy-display-simulator.exe"
$screenshotsDir = Join-Path $scriptDir "screenshots"
& $exe --screenshot-dir $screenshotsDir @args
