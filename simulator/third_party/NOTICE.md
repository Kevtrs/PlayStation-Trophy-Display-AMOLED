# Bibliotheques tierces vendorisees (simulateur uniquement)

## lvgl-9.5.0/ (LVGL 9.5.0)

Source : https://github.com/lvgl/lvgl (tag `v9.5.0`, commit `85aa60d`),
licence MIT. Le simulateur CMake pointe explicitement vers ce dossier pour
garantir que la preview UI compile et s'execute avec LVGL 9.5.0.

L'ancien dossier `lvgl/` (9.4.0) est conserve temporairement comme reference
locale afin de faciliter une comparaison ou un retour arriere pendant la phase
de validation.

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
