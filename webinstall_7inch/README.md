# Installation web (ESP Web Tools) -- Waveshare ESP32-S3-Touch-LCD-7

Meme principe que [`webinstall/`](../webinstall/) (board rond AMOLED 1.75)
mais pour le board large 800x480 -- voir `platformio.ini`,
`[env:waveshare-7inch-rgb]`. Flashe directement depuis Chrome/Edge (API Web
Serial), sans installer PlatformIO/Python.

**Materiel jamais teste sur un vrai board avant le premier flash reel**
(recu le 2026-07-28) : la compilation et le simulateur PC (SDL2) sont
verifies, mais le bring-up bas niveau (RGB/GT911/CH422G, voir
`src/main_7inch.cpp`) n'a jamais tourne sur silicium reel. Si l'ecran reste
noir ou se comporte bizarrement au premier flash, voir les commentaires de
bring-up en tete de `src/main_7inch.cpp` avant de soupconner ce script.

## Publier sur GitHub Pages

Meme depot/branche Pages que `webinstall/` -- ce dossier est simplement une
seconde page a la racine de la meme publication. Si Pages sert depuis
`/webinstall`, deplacez ou dupliquez la configuration pour exposer aussi
`/webinstall_7inch` (ou fusionnez les deux sous un choix de board sur une
page commune -- pas fait ici, decision volontairement laissee a
l'utilisateur).

## Regenerer les binaires apres une modification du firmware

```powershell
# Depuis la racine du depot :
pio run -e waveshare-7inch-rgb
pio run -e waveshare-7inch-rgb -t buildfs
.\webinstall_7inch\update_firmware.ps1
```

Puis incrementez `"version"` dans `manifest.json` et committez.

**Important (voir webinstall/README.md et AUDIT.md section 0quater)** : le
firmware compile integre la cle API Pocket PSN partagee
(`include/secrets.h`, jamais committee) -- ne regenerez et ne publiez ces
binaires que depuis une machine ou ce fichier contient la vraie cle, sinon
la version publiee retombera en mode demo pour tout le monde.

## Apres l'installation

La carte cree son propre reseau Wi-Fi `TrophyDisplay-Setup` tant qu'aucune
configuration n'est enregistree -- **et l'ecran 800x480 l'affiche
directement** (voir `build_wifi_setup_screen_wide()`, contrairement au
board rond qui ne montre qu'un badge "Hors ligne"). Connectez-vous a ce
reseau, une page de configuration (Wi-Fi + pseudo PSN) s'ouvre
automatiquement (sinon : `http://192.168.4.1/`).

## Pourquoi Chrome/Edge uniquement

Voir `webinstall/README.md` -- identique (Web Serial, navigateurs bases sur
Chromium uniquement, sur ordinateur).
