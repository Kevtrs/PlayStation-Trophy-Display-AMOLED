<#
.SYNOPSIS
    Sonde uniquement les ressources PUBLIQUES de pocketpsn.com (sans cle,
    sans authentification, sans contournement d'aucune protection).

.DESCRIPTION
    Enveloppe PowerShell autour de probe.py. Ne fait rien de plus que
    lancer une requete HTTP GET normale vers la page de profil publique et
    quelques ressources habituellement publiques du domaine, afin de
    determiner honnetement ce qui est recuperable sans cle privee.

.PARAMETER Psn
    Le pseudo PSN public a interroger (le votre).

.EXAMPLE
    .\run.ps1 -Psn "gaz91610"
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$Psn
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

$python = Get-Command python -ErrorAction SilentlyContinue
if (-not $python) {
    $python = Get-Command python3 -ErrorAction SilentlyContinue
}
if (-not $python) {
    Write-Error "Python 3 introuvable dans le PATH. Installez-le avant d'utiliser cet outil."
    exit 1
}

& $python.Source (Join-Path $scriptDir "probe.py") $Psn
exit $LASTEXITCODE
