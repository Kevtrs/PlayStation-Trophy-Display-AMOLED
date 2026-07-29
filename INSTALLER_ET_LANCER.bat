@echo off
REM Installation clef en main du firmware PlayStation Trophy Display AMOLED.
REM Double-cliquez sur ce fichier. Voir PREMIER_DEMARRAGE.md pour le detail.

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\installer_et_lancer.ps1" %*

echo.
echo ----------------------------------------------------
echo Fin du script. Fermez cette fenetre ou appuyez sur une touche.
echo ----------------------------------------------------
pause >nul
