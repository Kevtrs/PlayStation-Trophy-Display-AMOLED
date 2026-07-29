#Requires -Version 5.1
<#
Installation clef en main : verifie l'environnement, detecte la carte,
compile, flashe et ouvre le moniteur serie. Voir PREMIER_DEMARRAGE.md pour
le mode d'emploi complet.

Parametres optionnels (usage normal : aucun) :
  -DryRun       Fait tout sauf flasher/ouvrir le moniteur (verifie que la
                compilation et la detection de port fonctionnent, sans
                toucher a une carte reelle). Utile pour re-tester ce script
                sans materiel branche.
  -SkipFsUpload Saute l'ecriture LittleFS (data/) -- a utiliser uniquement
                si vous savez que la partition est deja a jour et voulez
                explicitement preserver la configuration deja enregistree
                sur la carte (sinon, le premier flash doit toujours inclure
                cette etape, voir docs/BUILD_FLASH_FIRSTBOOT.md).
  -EnvName      Environnement PlatformIO a compiler/flasher (defaut :
                "waveshare-amoled-175", le board rond). Utilise par
                INSTALLER_ET_LANCER_7POUCES.bat pour reutiliser ce meme
                script avec "waveshare-7inch-rgb" (voir platformio.ini)
                plutot que dupliquer toute cette logique.
#>
param(
    [switch]$DryRun,
    [switch]$SkipFsUpload,
    [string]$EnvName = "waveshare-amoled-175"
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "common.ps1")

$envName = $EnvName
$totalSteps = 7

Write-Banner "PLAYSTATION TROPHY DISPLAY -- INSTALLATION"
if ($DryRun) {
    Write-Host "(mode -DryRun : compilation et detection uniquement, pas de flash reel)" -ForegroundColor Yellow
    Write-Host ""
}

# --- [1/7] Python ---------------------------------------------------------
$pythonOk = Test-PythonAvailable
Write-StepResult "[1/$totalSteps] Verification de Python" $pythonOk
if (-not $pythonOk) {
    Write-ErrorBlock "PYTHON INTROUVABLE" @(
        "Aucune commande 'python' ou 'py' fonctionnelle n'a ete trouvee.",
        "",
        "Installez Python 3 depuis https://www.python.org/downloads/",
        "puis relancez ce script (redemarrer le PC apres l'installation",
        "peut etre necessaire pour que le PATH soit pris en compte)."
    )
    exit 1
}
Write-Info "  Commande Python utilisee : $script:PythonCmd"

# --- [2/7] PlatformIO -------------------------------------------------------
$pioOk = Test-PlatformIoAvailable
if (-not $pioOk) {
    Write-Info "  PlatformIO non detecte via '$script:PythonCmd -m platformio' -- tentative d'installation automatique."
    $pioOk = Install-PlatformIo
}
Write-StepResult "[2/$totalSteps] Verification de PlatformIO" $pioOk
if (-not $pioOk) {
    Write-ErrorBlock "PLATFORMIO INDISPONIBLE" @(
        "L'installation automatique de PlatformIO a echoue.",
        "",
        "Essayez manuellement dans un terminal :",
        "  $script:PythonCmd -m pip install --upgrade platformio",
        "",
        "Puis relancez ce script. Si l'erreur mentionne un probleme reseau/proxy,",
        "verifiez votre connexion internet (l'installation telecharge des paquets)."
    )
    exit 1
}

# --- [3/7] Port ESP32-S3 ---------------------------------------------------
$port = $null
if ($DryRun) {
    Write-Info "  (DryRun) detection de port testee, resultat ignore pour la suite."
    Get-CandidateEspPorts | ForEach-Object { Write-Info "    trouve: $($_.Port) -- $($_.Description) -- plausible=$($_.Plausible)" }
    Write-StepResult "[3/$totalSteps] Detection de l'ESP32-S3" $true
} else {
    $port = Select-EspPort
    Write-StepResult "[3/$totalSteps] Detection de l'ESP32-S3" ([bool]$port)
    if (-not $port) {
        exit 1
    }
    Write-Info "  Port retenu : $port"
}

# --- [4/7] Verification du projet ------------------------------------------
$layout = Test-ProjectLayout
Write-StepResult "[4/$totalSteps] Verification du projet" $layout.Ok
if (-not $layout.Ok) {
    Write-ErrorBlock "PROJET INCOMPLET" @(
        $layout.Reason,
        "",
        "Ce script doit etre lance depuis une copie complete et intacte du depot",
        "PlayStation-Trophy-Display-AMOLED (dossier contenant platformio.ini)."
    )
    exit 1
}

# --- [5/7] Compilation ------------------------------------------------------
$buildLog = New-LogPath "build"
Write-Info "  Compilation en cours (environnement '$envName')... journal : $buildLog"
$buildExit = Invoke-Pio -PioArgs @("run", "-e", $envName) -LogPath $buildLog

if ($buildExit -ne 0) {
    Write-Info "  Premiere compilation en echec -- nouvel essai apres un nettoyage complet (peut prendre plusieurs minutes)..."
    Invoke-Pio -PioArgs @("run", "-e", $envName, "-t", "clean") -LogPath $null | Out-Null
    $buildLog = New-LogPath "build_clean"
    $buildExit = Invoke-Pio -PioArgs @("run", "-e", $envName) -LogPath $buildLog
}

$buildOk = ($buildExit -eq 0)
Write-StepResult "[5/$totalSteps] Compilation du firmware" $buildOk

if (-not $buildOk) {
    Write-ErrorBlock "ECHEC DE LA COMPILATION" @(
        "La compilation a echoue meme apres un nettoyage complet.",
        "",
        "Log complet : $buildLog",
        "",
        "Les dernieres lignes du journal :"
    )
    Get-Content $buildLog -Tail 20 | ForEach-Object { Write-Host "  $_" }
    Write-Host ""
    Write-Host "Voir DEPANNAGE_MATERIEL.md (section Flash) si l'erreur n'est pas claire." -ForegroundColor Yellow
    exit 1
}

$summary = Get-BuildSummary $buildLog
Write-Info "  RAM: $($summary.Ram) | Flash: $($summary.Flash) | Warnings: $($summary.Warnings)"
if ($summary.Warnings -gt 0) {
    Write-Host "  Attention : $($summary.Warnings) warning(s) de compilation -- voir $buildLog" -ForegroundColor Yellow
}

if ($DryRun) {
    Write-Host ""
    Write-Host "Mode -DryRun : arret ici (pas de flash, pas de moniteur serie)." -ForegroundColor Yellow
    Write-Host "Relancez sans -DryRun, carte branchee, pour l'installation complete." -ForegroundColor Yellow
    exit 0
}

# --- [6/7] Flash -------------------------------------------------------------
$flashOk = $false
$maxAttempts = 3
$attempt = 0
# Deux fichiers de log DISTINCTS : Invoke-Pio redirige avec "*>" (ecrase le
# fichier a chaque appel, ne l'ajoute pas) -- reutiliser le meme chemin pour
# l'ecriture LittleFS puis le flash firmware effacait silencieusement le
# journal de la premiere etape avec celui de la seconde. Bug reel trouve le
# 2026-07-24 : l'ecriture LittleFS avait en realite echoue (port occupe par
# un moniteur serie laisse ouvert) mais rien ne le montrait dans le journal
# final, et le script continuait quand meme -- l'appareil tournait donc
# avec l'ancienne page web (ou aucune), sans que rien ne le signale.
$fsFlashLog = New-LogPath "flash_fs"
$flashLog = New-LogPath "flash"

if (-not $SkipFsUpload) {
    Write-Info "  Preparation du systeme de fichiers LittleFS (data/) -- construction..."
    Invoke-Pio -PioArgs @("run", "-e", $envName, "-t", "buildfs") -LogPath $null | Out-Null
    $fsOk = $false
    $fsAttempt = 0
    while (-not $fsOk -and $fsAttempt -lt $maxAttempts) {
        $fsAttempt++
        Write-Info "  Ecriture LittleFS (data/) sur $port (tentative $fsAttempt/$maxAttempts) -- efface toute configuration existante (sans consequence sur une carte neuve)..."
        $fsExit = Invoke-Pio -PioArgs @("run", "-e", $envName, "-t", "uploadfs", "--upload-port", $port) -LogPath $fsFlashLog
        $fsOk = ($fsExit -eq 0)
        if (-not $fsOk -and $fsAttempt -lt $maxAttempts) {
            Write-ErrorBlock "ECHEC DE L'ECRITURE LITTLEFS (data/)" @(
                "La carte est detectee sur $port mais l'ecriture du systeme de fichiers a echoue.",
                "Cause frequente : un moniteur serie ou un autre programme garde le port ouvert.",
                "",
                "Fermez toute autre fenetre utilisant $port (moniteur serie, autre instance de ce script...),",
                "puis appuyez sur Entree pour reessayer."
            )
            Read-Host | Out-Null
        }
    }
    if (-not $fsOk) {
        Write-ErrorBlock "ECRITURE LITTLEFS TOUJOURS EN ECHEC APRES $maxAttempts TENTATIVES" @(
            "Le firmware va quand meme etre flashe, mais le portail web (configuration",
            "Wi-Fi/Pocket PSN) risque de ne pas s'afficher correctement tant que ceci",
            "n'est pas resolu -- relancez ce script une fois le port libere.",
            "",
            "Log complet : $fsFlashLog"
        )
    } else {
        Write-Info "  Systeme de fichiers LittleFS ecrit avec succes."
    }
}

while (-not $flashOk -and $attempt -lt $maxAttempts) {
    $attempt++
    Write-Info "  Flash du firmware sur $port (tentative $attempt/$maxAttempts)..."
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

Write-StepResult "[6/$totalSteps] Flash du firmware" $flashOk

if (-not $flashOk) {
    Write-ErrorBlock "FLASH IMPOSSIBLE APRES $maxAttempts TENTATIVES" @(
        "Log complet : $flashLog",
        "",
        "Verifiez :",
        "  - Le cable USB (donnees, pas seulement charge).",
        "  - Qu'aucun autre programme n'utilise $port (moniteur serie, Arduino IDE...).",
        "  - Que la carte n'est pas endommagee (essayez un autre port/cable USB).",
        "",
        "Voir DEPANNAGE_MATERIEL.md, section Flash, pour le detail de chaque cas."
    )
    exit 1
}

# --- [7/7] Moniteur serie ----------------------------------------------------
Write-StepResult "[7/$totalSteps] Ouverture du moniteur serie" $true
Write-Host ""
Write-Host "Le firmware a ete installe avec succes." -ForegroundColor Green
Write-Host ""
Write-Host "Ouverture du moniteur serie (115200 bauds). Le tout premier demarrage"
Write-Host "peut prendre quelques secondes (initialisation ecran/tactile/Wi-Fi)."
Write-Host "Repere de progression attendu : [BOOT] System, Display, Touch, Config,"
Write-Host "Cache, Network, PocketPSN, UI ready (dans cet ordre)."
Write-Host ""
Write-Host "Appuyez sur Ctrl+C pour quitter le moniteur a tout moment." -ForegroundColor Yellow
Write-Host ""

$serialLog = New-LogPath "serial"
Write-Info "  Journal serie egalement enregistre dans : $serialLog"
Start-Sleep -Seconds 1

try {
    & $script:PythonCmd -m platformio device monitor -e $envName --port $port --baud 115200 --filter esp32_exception_decoder 2>&1 |
        Tee-Object -FilePath $serialLog
} catch {
    Write-Host ""
    Write-Host "Moniteur serie ferme." -ForegroundColor Gray
}
