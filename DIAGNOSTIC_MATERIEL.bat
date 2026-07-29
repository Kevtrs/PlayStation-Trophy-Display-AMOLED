@echo off
REM Diagnostic materiel independant (ecran/tactile/memoire/puce/Wi-Fi).
REM Double-cliquez sur ce fichier. Ne modifie jamais le firmware normal.
REM Pour revenir au firmware normal ensuite : INSTALLER_ET_LANCER.bat

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\diagnostic_materiel.ps1" %*

echo.
echo ----------------------------------------------------
echo Fin du script. Fermez cette fenetre ou appuyez sur une touche.
echo ----------------------------------------------------
pause >nul
