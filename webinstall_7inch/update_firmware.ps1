#Requires -Version 5.1
<#
Copie les binaires fraichement compiles (.pio/build/waveshare-7inch-rgb/)
dans webinstall_7inch/firmware/, pour publier une nouvelle version sur la
page d'installation web (voir webinstall_7inch/README.md). Meme logique que
webinstall/update_firmware.ps1 (board rond), adaptee a cet environnement --
inclut aussi littlefs.bin genere via "buildfs" (portail captif reel, voir
data/index.html), absent de la compilation "run" normale.

Usage : compilez d'abord ("pio run -e waveshare-7inch-rgb" et
"pio run -e waveshare-7inch-rgb -t buildfs"), puis lancez ce script depuis
la racine du depot. Pensez a incrementer "version" dans
webinstall_7inch/manifest.json avant de committer.
#>

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot ".pio\build\waveshare-7inch-rgb"
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
# "pio run". Cet environnement epingle un fork (pioarduino, voir
# platformio.ini) mais s'installe dans le meme dossier de paquet standard.
$pioPackages = Join-Path $env:USERPROFILE ".platformio\packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin"
$files[4].Source = $pioPackages

if (-not (Test-Path $destDir)) {
    New-Item -ItemType Directory -Path $destDir | Out-Null
}

foreach ($f in $files) {
    if (-not (Test-Path $f.Source)) {
        Write-Host "MANQUANT : $($f.Source) -- avez-vous bien compile (pio run -e waveshare-7inch-rgb) ET genere le systeme de fichiers (pio run -e waveshare-7inch-rgb -t buildfs) ?" -ForegroundColor Red
        exit 1
    }
    Copy-Item -Path $f.Source -Destination (Join-Path $destDir $f.Name) -Force
    Write-Host "Copie : $($f.Name)" -ForegroundColor Green
}

Write-Host ""
Write-Host "Binaires mis a jour dans webinstall_7inch/firmware/." -ForegroundColor Cyan
Write-Host "N'oubliez pas d'incrementer 'version' dans webinstall_7inch/manifest.json avant de committer." -ForegroundColor Yellow
