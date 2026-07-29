@echo off
REM Installation clef en main du firmware pour le board Waveshare
REM ESP32-S3-Touch-LCD-7 (800x480, ecran large -- voir platformio.ini,
REM [env:waveshare-7inch-rgb]). Double-cliquez sur ce fichier. Materiel
REM jamais teste avant le premier flash reel (recu le 2026-07-28) : si
REM l'ecran reste noir, voir les commentaires de bring-up en tete de
REM src/main_7inch.cpp.
REM
REM Reutilise scripts\installer_et_lancer.ps1 (meme logique deja fiable que
REM le board rond -- detection de port, compilation, ecriture LittleFS,
REM flash avec reessais, moniteur serie) avec -EnvName pour cibler ce board
REM plutot que le board rond.

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\installer_et_lancer.ps1" -EnvName waveshare-7inch-rgb %*

echo.
echo ----------------------------------------------------
echo Fin du script. Fermez cette fenetre ou appuyez sur une touche.
echo ----------------------------------------------------
pause >nul
