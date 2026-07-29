#Requires -Version 5.1
# Fonctions partagees entre installer_et_lancer.ps1 et diagnostic_materiel.ps1.
# Compatible Windows PowerShell 5.1 (pas d'operateur ternaire/null-coalescing).

$script:RepoRoot = Split-Path -Parent $PSScriptRoot
$script:PythonCmd = $null
$script:PioArgsPrefix = $null

function Write-Banner([string]$title) {
    Write-Host ""
    Write-Host "====================================================" -ForegroundColor Cyan
    Write-Host " $title" -ForegroundColor Cyan
    Write-Host "====================================================" -ForegroundColor Cyan
    Write-Host ""
}

function Write-StepResult([string]$label, [bool]$ok) {
    $dotsCount = 42 - $label.Length
    if ($dotsCount -lt 1) { $dotsCount = 1 }
    $dots = "." * $dotsCount
    Write-Host ("{0}{1} " -f $label, $dots) -NoNewline
    if ($ok) {
        Write-Host "OK" -ForegroundColor Green
    } else {
        Write-Host "ECHEC" -ForegroundColor Red
    }
}

function Write-Info([string]$text) {
    Write-Host $text -ForegroundColor Gray
}

function Write-ErrorBlock([string]$title, [string[]]$lines) {
    Write-Host ""
    Write-Host $title -ForegroundColor Red
    Write-Host ""
    foreach ($line in $lines) {
        Write-Host $line
    }
    Write-Host ""
}

function Ensure-Dir([string]$path) {
    if (-not (Test-Path $path)) {
        New-Item -ItemType Directory -Path $path -Force | Out-Null
    }
}

function New-LogPath([string]$prefix) {
    Ensure-Dir (Join-Path $RepoRoot "logs")
    $timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
    return Join-Path $RepoRoot "logs\$prefix`_$timestamp.log"
}

# --- 1. Environnement --------------------------------------------------

function Test-PythonAvailable {
    foreach ($candidate in @("python", "py")) {
        try {
            $version = & $candidate --version 2>&1
            if ($LASTEXITCODE -eq 0 -or $version -match "Python \d") {
                $script:PythonCmd = $candidate
                return $true
            }
        } catch {
            continue
        }
    }
    return $false
}

function Test-PlatformIoAvailable {
    if (-not $script:PythonCmd) { return $false }
    try {
        & $script:PythonCmd -m platformio --version 2>&1 | Out-Null
        return ($LASTEXITCODE -eq 0)
    } catch {
        return $false
    }
}

function Install-PlatformIo {
    Write-Info "PlatformIO absent -- installation via '$script:PythonCmd -m pip install -U platformio' (peut prendre quelques minutes)..."
    & $script:PythonCmd -m pip install --upgrade platformio 2>&1 | ForEach-Object { Write-Info "  $_" }
    if ($LASTEXITCODE -ne 0) {
        return $false
    }
    return (Test-PlatformIoAvailable)
}

# Toujours invoquer PlatformIO via "python -m platformio" (jamais "pio" nu) :
# le PATH ne l'expose pas forcement, meme quand l'installation a reussi.
function Invoke-Pio {
    param(
        [Parameter(Mandatory = $true)][string[]]$PioArgs,
        [string]$LogPath = $null
    )
    # Redirection de flux ("*>"), jamais un pipeline ("| Tee-Object") : un
    # pipeline laisse chaque objet transmis rejoindre le flux de sortie de
    # CETTE fonction elle-meme (PowerShell renvoie tout ce qui n'est pas
    # capture, pas seulement la valeur de "return"), ce qui transformait
    # silencieusement le code de sortie en tableau et cassait toute
    # comparaison booleenne en aval (bug reel trouve en testant ce script).
    #
    # $ErrorActionPreference locale (n'affecte pas l'appelant, PowerShell la
    # limite au scope de cette fonction) : avec "Stop" (herite des scripts
    # appelants), la moindre ligne ecrite par esptool sur stderr -- ce qui
    # arrive des qu'un flash echoue, cas normal a couvrir -- devient une
    # exception PowerShell qui tue tout le script au lieu de laisser le
    # code de sortie et la boucle de nouvelle tentative faire leur travail.
    # Bug reel trouve en testant un vrai echec de flash materiel.
    $ErrorActionPreference = "Continue"
    $fullArgs = @("-m", "platformio") + $PioArgs
    if ($LogPath) {
        & $script:PythonCmd @fullArgs *> $LogPath
    } else {
        & $script:PythonCmd @fullArgs *> $null
    }
    return $LASTEXITCODE
}

function Test-ProjectLayout {
    $iniPath = Join-Path $RepoRoot "platformio.ini"
    if (-not (Test-Path $iniPath)) {
        return @{ Ok = $false; Reason = "platformio.ini introuvable a la racine du depot ($RepoRoot)." }
    }
    $iniContent = Get-Content $iniPath -Raw
    if ($iniContent -notmatch "\[env:waveshare-amoled-175\]") {
        return @{ Ok = $false; Reason = "L'environnement [env:waveshare-amoled-175] est absent de platformio.ini." }
    }
    $requiredPaths = @("src\main.cpp", "src\app\AppController.cpp", "include\BoardConfig.h", "data\index.html")
    $missing = @()
    foreach ($rel in $requiredPaths) {
        if (-not (Test-Path (Join-Path $RepoRoot $rel))) {
            $missing += $rel
        }
    }
    if ($missing.Count -gt 0) {
        return @{ Ok = $false; Reason = "Fichier(s) essentiel(s) manquant(s) : " + ($missing -join ", ") }
    }
    return @{ Ok = $true; Reason = "" }
}

# --- 2. Detection du port serie -----------------------------------------

# Mots-cles observes sur les adaptateurs USB-serie couramment utilises par
# les cartes ESP32-S3 (CP210x, CH340, USB JTAG/serial natif, Espressif).
$script:EspPortKeywords = @(
    "CP210", "Silicon Labs", "CH340", "CH9102", "USB-SERIAL", "USB Serial",
    "USB JTAG", "Espressif", "USB-Enhanced-SERIAL"
)

function Get-CandidateEspPorts {
    $ports = @()
    try {
        $pnpEntities = Get-CimInstance -ClassName Win32_PnPEntity -ErrorAction Stop |
            Where-Object { $_.Name -match "\(COM\d+\)" }
    } catch {
        $pnpEntities = @()
    }

    foreach ($entity in $pnpEntities) {
        if ($entity.Name -match "\((COM\d+)\)") {
            $comName = $Matches[1]
            $isPlausible = $false
            foreach ($keyword in $script:EspPortKeywords) {
                if ($entity.Name -match [regex]::Escape($keyword) -or
                    ($entity.Description -and $entity.Description -match [regex]::Escape($keyword))) {
                    $isPlausible = $true
                    break
                }
            }
            $ports += [PSCustomObject]@{
                Port        = $comName
                Description = $entity.Name
                Plausible   = $isPlausible
            }
        }
    }

    # Filet de securite : si la requete WMI ne renvoie rien (pilote non
    # reconnu) mais qu'un port serie existe bien au niveau .NET, le proposer
    # quand meme plutot que de pretendre qu'aucun port n'est present.
    if ($ports.Count -eq 0) {
        foreach ($name in [System.IO.Ports.SerialPort]::GetPortNames()) {
            $ports += [PSCustomObject]@{ Port = $name; Description = "$name (description indisponible)"; Plausible = $false }
        }
    }

    return $ports
}

# Retourne le nom du port choisi, ou $null si aucun port disponible/choisi.
# Ne choisit JAMAIS silencieusement en cas d'ambiguite reelle (plusieurs
# ports plausibles) : affiche une liste numerotee et demande explicitement.
function Select-EspPort {
    # "@(...)" force le contexte tableau : sans ca, PowerShell 5.1 renvoie
    # parfois un objet scalaire (pas un tableau a 1 element) quand une seule
    # valeur traverse "return"/le pipeline, et ".Count" devient alors $null
    # au lieu de 1 -- bug reel trouve en testant ce script avec un seul port
    # plausible (le cas le plus probable demain), qui faisait rater
    # l'auto-selection et tomber dans le mauvais embranchement.
    $ports = @(Get-CandidateEspPorts)

    if ($ports.Count -eq 0) {
        Write-ErrorBlock "AUCUN PORT SERIE DETECTE" @(
            "Verifiez que :",
            "  1. Le cable USB utilise supporte bien les donnees (pas seulement la charge).",
            "  2. La carte ESP32-S3 est bien branchee et sous tension (LED allumee).",
            "  3. Les pilotes USB-UART sont installes (CP210x ou CH340 selon la carte).",
            "     Waveshare fournit generalement un pilote CP210x pour cette carte.",
            "  4. Le port n'est pas deja ouvert par un autre programme (moniteur serie, Arduino IDE...).",
            "",
            "Rebranchez la carte puis relancez ce script."
        )
        return $null
    }

    $plausible = @($ports | Where-Object { $_.Plausible })

    if ($plausible.Count -eq 1) {
        Write-Info "Port plausible unique detecte : $($plausible[0].Port) ($($plausible[0].Description))"
        return $plausible[0].Port
    }

    $candidates = @(if ($plausible.Count -gt 1) { $plausible } else { $ports })

    Write-Host ""
    if ($plausible.Count -gt 1) {
        Write-Host "Plusieurs ports plausibles detectes -- choisissez le bon :" -ForegroundColor Yellow
    } else {
        Write-Host "Aucun port reconnu avec certitude comme ESP32 -- choisissez parmi les ports serie disponibles :" -ForegroundColor Yellow
    }
    for ($i = 0; $i -lt $candidates.Count; $i++) {
        Write-Host ("  [{0}] {1} -- {2}" -f ($i + 1), $candidates[$i].Port, $candidates[$i].Description)
    }
    Write-Host ""
    $choice = Read-Host "Entrez le numero du port a utiliser (ou appuyez sur Entree pour annuler)"
    if ([string]::IsNullOrWhiteSpace($choice)) { return $null }
    $index = 0
    if ([int]::TryParse($choice, [ref]$index) -and $index -ge 1 -and $index -le $candidates.Count) {
        return $candidates[$index - 1].Port
    }
    Write-Host "Choix invalide." -ForegroundColor Red
    return $null
}

# --- 3. Resume de build --------------------------------------------------

function Get-BuildSummary([string]$logPath) {
    $content = Get-Content $logPath -Raw -ErrorAction SilentlyContinue
    if (-not $content) { return @{ Ram = "?"; Flash = "?"; Warnings = 0 } }
    $ram = "?"
    $flash = "?"
    if ($content -match "RAM:\s*\[[=\s]*\]\s*([\d.]+%.*)") { $ram = $Matches[1].Trim() }
    if ($content -match "Flash:\s*\[[=\s]*\]\s*([\d.]+%.*)") { $flash = $Matches[1].Trim() }
    $warningMatches = [regex]::Matches($content, "warning:")
    return @{ Ram = $ram; Flash = $flash; Warnings = $warningMatches.Count }
}
