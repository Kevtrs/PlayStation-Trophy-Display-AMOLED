#Requires -Version 5.1
<#
Diagnostic materiel independant du firmware normal (voir
src_diagnostic/main.cpp) : mires ecran (couleurs, degrade, cercle, reperes
de coin, texte centre, rotation), test tactile en direct, infos puce/memoire,
scan Wi-Fi minimal. N'utilise jamais LVGL/AppController/le design de l'interface --
si ce firmware fonctionne mais pas le firmware normal, le probleme est
logiciel (LVGL) plutot que materiel.

Parametres optionnels :
  -DryRun           Compile et verifie la detection de port sans flasher.
  -RestoreNormal    Reflashe directement le firmware NORMAL (raccourci
                     pratique apres un diagnostic, equivalent a relancer
                     INSTALLER_ET_LANCER.bat).
#>
param(
    [switch]$DryRun,
    [switch]$RestoreNormal
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "common.ps1")

if ($RestoreNormal) {
    Write-Banner "RESTAURATION DU FIRMWARE NORMAL"
    & (Join-Path $PSScriptRoot "installer_et_lancer.ps1")
    exit $LASTEXITCODE
}

$envName = "waveshare-amoled-175-diagnostic"
$totalSteps = 6

Write-Banner "PLAYSTATION TROPHY DISPLAY -- DIAGNOSTIC MATERIEL"
Write-Host "Ce firmware de test est INDEPENDANT du firmware normal : il ne modifie"
Write-Host "ni ne remplace votre configuration. Pour revenir au firmware normal"
Write-Host "ensuite, relancez INSTALLER_ET_LANCER.bat (ou ce script avec -RestoreNormal)."
if ($DryRun) {
    Write-Host ""
    Write-Host "(mode -DryRun : compilation et detection uniquement, pas de flash reel)" -ForegroundColor Yellow
}
Write-Host ""

# --- [1/6] Python, [2/6] PlatformIO ------------------------------------------
$pythonOk = Test-PythonAvailable
Write-StepResult "[1/$totalSteps] Verification de Python" $pythonOk
if (-not $pythonOk) {
    Write-ErrorBlock "PYTHON INTROUVABLE" @("Installez Python 3 puis relancez ce script (voir INSTALLER_ET_LANCER.bat pour le detail).")
    exit 1
}

$pioOk = Test-PlatformIoAvailable
if (-not $pioOk) { $pioOk = Install-PlatformIo }
Write-StepResult "[2/$totalSteps] Verification de PlatformIO" $pioOk
if (-not $pioOk) {
    Write-ErrorBlock "PLATFORMIO INDISPONIBLE" @("Voir INSTALLER_ET_LANCER.bat pour la procedure d'installation manuelle.")
    exit 1
}

# --- [3/6] Port ---------------------------------------------------------------
$port = $null
if ($DryRun) {
    Get-CandidateEspPorts | ForEach-Object { Write-Info "  trouve: $($_.Port) -- $($_.Description) -- plausible=$($_.Plausible)" }
    Write-StepResult "[3/$totalSteps] Detection de l'ESP32-S3" $true
} else {
    $port = Select-EspPort
    Write-StepResult "[3/$totalSteps] Detection de l'ESP32-S3" ([bool]$port)
    if (-not $port) { exit 1 }
}

# --- [4/6] Compilation ---------------------------------------------------------
$buildLog = New-LogPath "diag_build"
Write-Info "  Compilation du firmware de diagnostic ('$envName')... journal : $buildLog"
$buildExit = Invoke-Pio -PioArgs @("run", "-e", $envName) -LogPath $buildLog
$buildOk = ($buildExit -eq 0)
Write-StepResult "[4/$totalSteps] Compilation du diagnostic" $buildOk
if (-not $buildOk) {
    Write-ErrorBlock "ECHEC DE LA COMPILATION DU DIAGNOSTIC" @(
        "Log complet : $buildLog",
        "Dernieres lignes :"
    )
    Get-Content $buildLog -Tail 20 | ForEach-Object { Write-Host "  $_" }
    exit 1
}
$summary = Get-BuildSummary $buildLog
Write-Info "  RAM: $($summary.Ram) | Flash: $($summary.Flash) | Warnings: $($summary.Warnings)"

if ($DryRun) {
    Write-Host ""
    Write-Host "Mode -DryRun : arret ici (pas de flash, pas de moniteur serie)." -ForegroundColor Yellow
    exit 0
}

# --- [5/6] Flash -----------------------------------------------------------------
$flashLog = New-LogPath "diag_flash"
$flashOk = $false
$maxAttempts = 3
$attempt = 0
while (-not $flashOk -and $attempt -lt $maxAttempts) {
    $attempt++
    Write-Info "  Flash du diagnostic sur $port (tentative $attempt/$maxAttempts)..."
    $flashExit = Invoke-Pio -PioArgs @("run", "-e", $envName, "-t", "upload", "--upload-port", $port) -LogPath $flashLog
    $flashOk = ($flashExit -eq 0)
    if (-not $flashOk -and $attempt -lt $maxAttempts) {
        Write-ErrorBlock "ECHEC DU FLASH" @(
            "La carte est detectee sur $port mais ne repond pas au bootloader.",
            "",
            "Procedure :",
            "1. Maintenez le bouton BOOT.",
            "2. Appuyez brievement sur RESET.",
            "3. Relachez RESET.",
            "4. Relachez BOOT.",
            "5. Appuyez sur Entree pour reessayer."
        )
        Read-Host | Out-Null
    }
}
Write-StepResult "[5/$totalSteps] Flash du diagnostic" $flashOk
if (-not $flashOk) {
    Write-ErrorBlock "FLASH IMPOSSIBLE APRES $maxAttempts TENTATIVES" @("Log complet : $flashLog", "Voir DEPANNAGE_MATERIEL.md.")
    exit 1
}

# --- [6/6] Moniteur ----------------------------------------------------------------
Write-StepResult "[6/$totalSteps] Ouverture du moniteur serie" $true
Write-Host ""
Write-Host "Sequence attendue a l'ecran (environ 2.5 s chacune, puis test tactile en continu) :"
Write-Host "  rouge -> vert -> bleu -> blanc -> noir -> degrade -> cercle ->"
Write-Host "  reperes de coin + texte '466 x 466 OK' -> info rotation -> test tactile."
Write-Host "Touchez l'ecran a tout moment pour sauter directement au test tactile."
Write-Host ""
Write-Host "Appuyez sur Ctrl+C pour quitter le moniteur." -ForegroundColor Yellow
Write-Host ""
Write-Host "Pour revenir au firmware normal ensuite :" -ForegroundColor Cyan
Write-Host "  INSTALLER_ET_LANCER.bat" -ForegroundColor Cyan
Write-Host ""

$serialLog = New-LogPath "diag_serial"
Write-Info "  Journal serie enregistre dans : $serialLog"
Start-Sleep -Seconds 1

try {
    & $script:PythonCmd -m platformio device monitor -e $envName --port $port --baud 115200 2>&1 |
        Tee-Object -FilePath $serialLog
} catch {
    Write-Host ""
    Write-Host "Moniteur serie ferme." -ForegroundColor Gray
}
