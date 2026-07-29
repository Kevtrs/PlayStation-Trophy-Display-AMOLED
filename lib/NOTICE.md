# Bibliotheques tierces vendorisees

## Arduino_GFX_Library (GFX Library for Arduino)

- Source : https://github.com/moononournation/Arduino_GFX
- Version vendorisee : v1.6.1 (commit a205402), licence BSD (voir `Arduino_GFX_Library/license.txt`)
- Copyright original : Adafruit Industries + moononournation

Modification appliquee (documentee ici conformement a la licence BSD) :
- Suppression de `src/databus/Arduino_ESP32RGBPanel.{h,cpp}` et de
  `src/display/Arduino_RGB_Display.{h,cpp}` (qui en depend directement), et
  de leurs lignes d'inclusion dans `src/Arduino_GFX_Library.h`.
- Raison : cette classe reconstruit manuellement la structure privee et non
  stable-ABI `esp_rgb_panel_t` du composant `esp_lcd` d'ESP-IDF. La version
  d'ESP-IDF livree avec `framework-arduinoespressif32 @ 3.20017.241212+sha.dcc1105b`
  (paquet resolu par PlatformIO au 2026-07-13 pour `espressif32@7.0.1`) a une
  disposition differente, ce qui provoque une erreur de compilation reelle
  (`error: 'esp_rgb_panel_t' was not declared in this scope`).
- Cette classe sert uniquement aux panneaux RGB paralleles / MIPI-DSI. Notre
  ecran (CO5300, bus QSPI) utilise `Arduino_ESP32QSPI` + `Arduino_CO5300`,
  non affectes par cette suppression.
- Version 1.6.1 choisie (plutot que 1.6.4, celle vendorisee par Waveshare)
  car c'est la derniere version de la librairie a la fois compatible avec
  le pilote CO5300 et sans dependance a `esp32-hal-periman.h`, absent du
  paquet framework installe ici (voir `platformio.ini` pour le detail).

Si le paquet PlatformIO `framework-arduinoespressif32` est corrige/mis a jour
un jour pour inclure `esp32-hal-periman.h` et une struct `esp_rgb_panel_t` a
jour, il redevient possible de revenir a une dependance en ligne sur une
version plus recente de la librairie officielle.
