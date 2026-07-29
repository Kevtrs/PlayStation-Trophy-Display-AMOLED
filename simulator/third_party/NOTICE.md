# Bibliotheques tierces vendorisees (simulateur uniquement)

## lvgl/ (LVGL 9.4.0)

Source : https://github.com/lvgl/lvgl (tag `v9.4.0`), licence MIT. Migre
depuis 8.3.11 le 2026-07-22 (voir HANDOFF_PROGRESS.md, branche
integration/final-lvgl-ui) pour adopter le design final, ecrit
nativement pour LVGL 9. Copie identique a celle utilisee par le firmware
ESP32 (`platformio.ini` epingle la meme version `9.4.0`). Dossiers `demos/`,
`docs/`, `env_support/`, `examples/`, `configs/` retires (non utilises ici,
uniquement pour reduire la taille du depot) -- seuls `src/` et les fichiers
racine necessaires a la compilation (`lvgl.h`, `lvgl_private.h`,
`lv_version.h`, etc.) sont conserves.

## stb/stb_image_write.h

Source : https://github.com/nothings/stb, domaine public / licence MIT au
choix. Utilise uniquement pour exporter les captures d'ecran PNG
(`simulator/screenshots/`), sans dependance externe (zlib deflate integre).

## SDL2/ (SDL 2.30.9, sous-ensemble `x86_64-w64-mingw32`)

Source : https://github.com/libsdl-org/SDL, release `SDL2-devel-2.30.9-mingw`,
licence zlib (voir `SDL2/LICENSE.txt`). Seul le sous-dossier
`x86_64-w64-mingw32` (include + lib d'import + `SDL2.dll` redistribuable) est
conserve, compatible avec la chaine de compilation `zig cc` ciblant
`x86_64-windows-gnu` utilisee par `run.ps1`/`build.ps1` (voir
`simulator/README.md`).

## ArduinoJson/ArduinoJson.h (ArduinoJson 7.4.3, en-tete unique)

Source : https://github.com/bblanchon/ArduinoJson, release
`v7.4.3` (`ArduinoJson-v7.4.3.h`), licence MIT. Meme version que celle
resolue par PlatformIO pour le firmware (`bblanchon/ArduinoJson@^7.1.0` ->
7.4.3), afin que `src/config/ConfigManager.cpp` et
`src/data/PocketPsnParser.cpp` (code de parsing/serialisation portable)
compilent a l'identique des deux cotes, sans toucher a Arduino/HTTPClient.
