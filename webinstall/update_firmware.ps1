#Requires -Version 5.1
<#
Copie les binaires fraichement compiles (.pio/build/waveshare-amoled-175/)
dans webinstall/firmware/, pour publier une nouvelle version sur la page
d'installation web (voir webinstall/README.md).

Usage : compilez d'abord ("pio run -e waveshare-amoled-175" ou
INSTALLER_ET_LANCER.bat), puis lancez ce script depuis la racine du depot.
Pensez a incrementer "version" dans webinstall/manifest.json avant de
committer.
#>

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot ".pio\build\waveshare-amoled-175"
$destDir = Join-Path $PSScriptRoot "firmware"

$files = @(
    @{ Name = "bootloader.bin"; Source = Join-Path $buildDir "bootloader.bin" },
    @{ Name = "partitions.bin"; Source = Join-Path $buildDir "partitions.bin" },
    @{ Name = "firmware.bin";   Source = Join-Path $buildDir "firmware.bin" },
    @{ Name = "littlefs.bin";   Source = Join-Path $buildDir "littlefs.bin" },
    @{ Name = "boot_app0.bin";  Source = $null }
)

# boot_app0.bin vient du framework Arduino-ESP32 (pas du dossier de build du
# projet) -- meme fichier pour toutes les compilations, jamais regenere par
# "pio run".
$pioPackages = Join-Path $env:USERPROFILE ".platformio\packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin"
$files[4].Source = $pioPackages

if (-not (Test-Path $destDir)) {
    New-Item -ItemType Directory -Path $destDir | Out-Null
}

foreach ($f in $files) {
    if (-not (Test-Path $f.Source)) {
        Write-Host "MANQUANT : $($f.Source) -- avez-vous bien compile le projet ?" -ForegroundColor Red
        exit 1
    }
    Copy-Item -Path $f.Source -Destination (Join-Path $destDir $f.Name) -Force
    Write-Host "Copie : $($f.Name)" -ForegroundColor Green
}

Write-Host ""
Write-Host "Binaires mis a jour dans webinstall/firmware/." -ForegroundColor Cyan
Write-Host "N'oubliez pas d'incrementer 'version' dans webinstall/manifest.json avant de committer." -ForegroundColor Yellow
