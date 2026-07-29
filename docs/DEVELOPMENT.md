# Choix de framework et journal des decisions techniques

## Framework retenu

**PlatformIO + framework Arduino (arduino-esp32) + GFX Library for Arduino + LVGL** (LVGL arrive en Phase 3).

Justification : Waveshare fournit officiellement des exemples Arduino IDE et
ESP-IDF pour cette carte (`waveshareteam/ESP32-S3-Touch-AMOLED-1.75`). Les
exemples Arduino utilisent exactement cette combinaison (`Arduino_GFX` pour
le CO5300 en QSPI, `SensorLib`/`TouchDrvCST92xx` pour le tactile, `lvgl` 8.x
pour l'UI). C'est donc la voie la mieux documentee et la plus testee par le
fabricant, conformement a la regle « privilegier la fiabilite ».

## Broches et pilotes -- provenance exacte

Toutes les valeurs dans [`include/BoardConfig.h`](../include/BoardConfig.h)
proviennent telles quelles de :
`waveshareteam/ESP32-S3-Touch-AMOLED-1.75/examples/arduino/libraries/Mylibrary/pin_config.h`

La sequence d'initialisation de l'ecran (bus `Arduino_ESP32QSPI` + panneau
`Arduino_CO5300`) et du tactile (`TouchDrvCST92xx` via `SensorLib`) reprend
`examples/arduino/01_HelloWorld` et `examples/arduino/10_Touch_CST9217` du
meme depot. Rien n'a ete invente ou devine.

## Incident de compatibilite reel rencontre (et sa resolution)

En tentant une compilation reelle (`pio run`) le 2026-07-13, deux problemes
concrets sont apparus avec le paquet `framework-arduinoespressif32` resolu
par defaut par PlatformIO pour `espressif32@7.0.1`
(`3.20017.241212+sha.dcc1105b`) :

1. `cores/esp32/esp32-hal-periman.h` est absent de ce paquet, alors que
   `GFX Library for Arduino >= 1.6.2` en depend inconditionnellement des
   que la cible est un ESP32-Sx/Cx (meme si on n'utilise que le bus QSPI).
   Essayer de forcer un arduino-esp32 3.x officiel via `platform_packages`
   echoue a son tour sur un desaccord de version de toolchain
   riscv32/xtensa non publiee sous cette forme dans le registry PlatformIO.
2. `Arduino_ESP32RGBPanel.cpp` (classe pour panneaux RGB paralleles/MIPI-DSI,
   non utilisee ici) ne compile pas avec l'ESP-IDF de ce paquet
   (`esp_rgb_panel_t` inconnu -- structure interne dont la disposition a
   change entre versions d'IDF).

**Resolution appliquee** : `GFX Library for Arduino` est vendorisee
localement dans [`lib/Arduino_GFX_Library`](../lib/Arduino_GFX_Library) a la
version v1.6.1 (derniere version compatible CO5300 sans dependre de
`esp32-hal-periman.h`), avec les fichiers `Arduino_ESP32RGBPanel.*` et
`Arduino_RGB_Display.*` retires (inutilises par notre bus QSPI). Voir
[`lib/NOTICE.md`](../lib/NOTICE.md) pour le detail exact et la justification
licence (BSD, modification documentee).

Ceci a ete verifie par une compilation reelle et reussie
(`pio run` -> `SUCCESS`, firmware.bin genere, Flash 11.3%/RAM 6.2% utilises
sur la partition `app0` de 3 Mo). **Cela ne remplace pas un test materiel** :
aucune carte physique n'etait disponible dans cet environnement d'agent pour
flasher et verifier l'ecran/le tactile reels -- a faire par l'utilisateur.

## Simulateur PC (Phase 3)

Chaine de compilation choisie : **`zig cc`/`zig c++` (paquet pip `ziglang`) +
CMake + Ninja (pip)**, plutot que Visual Studio ou MSYS2. Raison : reproductible
en une commande (`simulator/run.ps1`) sans installation systeme prealable,
verifie par une compilation reelle complete dans cet environnement (SDL2
vendorise en `x86_64-w64-mingw32`, LVGL 8.3.11 identique au firmware). Un
utilisateur avec Visual Studio/vcpkg deja installes peut ignorer ce script et
utiliser sa propre chaine (voir `simulator/README.md`).

`LV_MEM_SIZE` (`include/lv_conf.h`) est passe de 64 Ko a 160 Ko apres un
crash reel (`realloc` du pool interne LVGL echouant silencieusement avec 6
ecrans + panneau de debug). Ce changement s'applique aussi au firmware
(fichier partage) : impact RAM ESP32-S3 mesure a 56.8% (186 Ko/327 Ko) --
voir PROJECT_STATUS.md.

## Autres bibliotheques

- `SensorLib` (lewisxhe) : pas de tag Git publie, epingle sur la branche par
  defaut (HEAD) faute de mieux -- a re-verifier si une mise a jour amont
  casse le tactile CST9217.
- `XPowersLib` (lewisxhe) : epingle sur le tag `v0.2.6`, identique a la
  version vendorisee par Waveshare.
