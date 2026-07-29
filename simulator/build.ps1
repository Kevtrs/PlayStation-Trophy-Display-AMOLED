# Compile le simulateur PC (PowerShell, Windows).
# Chaine de compilation reproductible sans Visual Studio ni MSYS2 : Python +
# `pip install ziglang cmake ninja` fournissent un compilateur (zig cc/c++),
# CMake et Ninja. Voir simulator/README.md pour le detail et les alternatives
# (Visual Studio / vcpkg) si vous preferez votre propre chaine de compilation.

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

function Test-Command($name) {
    return [bool](Get-Command $name -ErrorAction SilentlyContinue)
}

if (-not (Test-Command "python")) {
    Write-Error "Python introuvable. Installez Python 3.10+ (python.org) puis relancez ce script."
    exit 1
}

Write-Host "Verification des outils de compilation (ziglang, cmake, ninja)..."
python -c "import ziglang, cmake, ninja" 2>$null
if ($LASTEXITCODE -ne 0) {
    Write-Host "Installation de ziglang, cmake, ninja via pip (une seule fois, quelques minutes)..."
    python -m pip install --quiet ziglang cmake ninja
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Echec de l'installation pip. Verifiez votre connexion internet."
        exit 1
    }
}

# Le dossier "Scripts" de Python contient cmake.exe/ninja.exe installes par
# pip : on l'ajoute au PATH de cette session pour que CMake trouve Ninja.
$scriptsDir = python -c "import sysconfig; print(sysconfig.get_path('scripts'))"
$env:PATH = "$scriptsDir;$env:PATH"

$buildDir = Join-Path $scriptDir "build"
$toolchainFile = Join-Path $scriptDir "toolchain\zig-toolchain.cmake"
$ninjaExe = Join-Path $scriptsDir "ninja.exe"

if (-not (Test-Path $buildDir)) {
    Write-Host "Configuration CMake (premiere fois)..."
    cmake -S $scriptDir -B $buildDir -G Ninja `
        "-DCMAKE_MAKE_PROGRAM=$ninjaExe" `
        "-DCMAKE_TOOLCHAIN_FILE=$toolchainFile"
    if ($LASTEXITCODE -ne 0) { exit 1 }
}

Write-Host "Compilation..."
cmake --build $buildDir
if ($LASTEXITCODE -ne 0) { exit 1 }

Write-Host "OK : $buildDir\trophy-display-simulator.exe"
