cmake_minimum_required(VERSION 3.16)
project(trophy_display_simulator C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(SDL2_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/third_party/SDL2)
set(LVGL_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/third_party/lvgl)

# --- LVGL (identique a la version epinglee par le firmware, voir
# third_party/NOTICE.md) : compilee ici comme simple bibliotheque C. ---
file(GLOB_RECURSE LVGL_SOURCES ${LVGL_ROOT}/src/*.c)
add_library(lvgl STATIC ${LVGL_SOURCES})
target_include_directories(lvgl PUBLIC
    ${LVGL_ROOT}
    ${REPO_ROOT}/include  # lv_conf.h partage avec le firmware
)
target_compile_definitions(lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE)

# --- Code UI partage avec le firmware ESP32-S3 (ne pas dupliquer) ---
# Design final (voir HANDOFF_PROGRESS.md, etape "integration ecrans") :
# theme/widgets/screens/assets + facade ui/trophy_display_ui, plus
# RoundUiBridge (adaptateur AppState -> modele du design, seul fichier
# specifique a ce projet dans cette liste).
file(GLOB ROUND_UI_SOURCES
    ${REPO_ROOT}/src/theme/theme.cpp
    ${REPO_ROOT}/src/widgets/widgets.cpp
    ${REPO_ROOT}/src/screens/screens.cpp
    ${REPO_ROOT}/src/assets/lucide_icons.cpp
    ${REPO_ROOT}/src/assets/pocketpsn_logo.cpp
    ${REPO_ROOT}/src/assets/premium_trophies.cpp
    ${REPO_ROOT}/src/assets/fonts/*.c
    ${REPO_ROOT}/src/ui/app.cpp
    ${REPO_ROOT}/src/ui/ui_model.cpp
    ${REPO_ROOT}/src/ui/trophy_display_ui.cpp
    ${REPO_ROOT}/src/ui/demo_data.cpp
    ${REPO_ROOT}/src/ui/ui_fixtures.cpp
    ${REPO_ROOT}/src/ui/strings.cpp
)
set(UI_SOURCES ${ROUND_UI_SOURCES} ${REPO_ROOT}/src/ui/RoundUiBridge.cpp ${REPO_ROOT}/src/ui/GestureRecognizer.cpp)
set(DATA_SOURCES ${REPO_ROOT}/src/data/DemoDataProvider.cpp)
# PocketPsnProvider.cpp est desormais portable (voir IPocketPsnHttpClient) :
# la couche transport HTTPClient/WiFiClientSecure a ete extraite dans
# src/network/PocketPsnHttpClient.cpp (firmware uniquement, exclu ici),
# remplacee ici par simulator/src/PocketPsnHttpClientStub.cpp. Inclus dans
# SHARED_LOGIC_SOURCES plus bas.

# --- Logique portable partagee avec le firmware (voir docs/ARCHITECTURE.md) :
# ne doit jamais dependre de LVGL, Arduino, HTTPClient ou LittleFS. ---
file(GLOB SHARED_LOGIC_SOURCES
    ${REPO_ROOT}/src/utils/Logger.cpp
    ${REPO_ROOT}/src/config/ConfigManager.cpp
    ${REPO_ROOT}/src/storage/FilePersistentStore.cpp
    ${REPO_ROOT}/src/storage/TrophyCache.cpp
    ${REPO_ROOT}/src/data/PocketPsnParser.cpp
    ${REPO_ROOT}/src/data/PocketPsnHtmlParser.cpp
    ${REPO_ROOT}/src/data/PocketPsnProvider.cpp
    ${REPO_ROOT}/src/data/ProviderFactory.cpp
    ${REPO_ROOT}/src/data/TrophyRepository.cpp
    ${REPO_ROOT}/src/services/PowerManager.cpp
    ${REPO_ROOT}/src/services/TimeService.cpp
    ${REPO_ROOT}/src/services/SyncService.cpp
    ${REPO_ROOT}/src/app/AppController.cpp
    ${REPO_ROOT}/src/web/WebApiHandlers.cpp
)
# NvsPersistentStore.cpp, network/WiFiManager.cpp et
# web/CaptivePortalServer.cpp sont volontairement exclus ici : backends
# LittleFS / WiFi.h / WebServer.h / DNSServer.h, firmware uniquement (voir
# simulator/src/WiFiManagerStub.cpp pour l'equivalent simulateur -- pas
# d'equivalent simulateur pour CaptivePortalServer, mais sa logique JSON
# portable -- WebApiHandlers -- est testee via --selftest).

add_executable(trophy-display-simulator
    src/main.cpp
    src/DisplayDriverSdl.cpp
    src/TouchDriverSdl.cpp
    src/DebugPanel.cpp
    src/WiFiManagerStub.cpp
    src/PocketPsnHttpClientStub.cpp
    src/ShowroomScenario.cpp
    ${UI_SOURCES}
    ${DATA_SOURCES}
    ${SHARED_LOGIC_SOURCES}
)

target_include_directories(trophy-display-simulator PRIVATE
    ${REPO_ROOT}/include
    ${REPO_ROOT}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ArduinoJson
    ${SDL2_ROOT}/include/SDL2
)

target_link_libraries(trophy-display-simulator PRIVATE
    lvgl
    ${SDL2_ROOT}/lib/libSDL2.dll.a
    winmm setupapi ole32 oleaut32 imm32 version uuid gdi32 user32
)

# Copie SDL2.dll a cote de l'executable (necessaire au lancement sur Windows).
add_custom_command(TARGET trophy-display-simulator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SDL2_ROOT}/bin/SDL2.dll
        $<TARGET_FILE_DIR:trophy-display-simulator>
)

# --- Mini-simulateur de prevue pour le board Waveshare ESP32-S3-Touch-LCD-7
# (800x480, rectangulaire, voir tache "board-7inch-rgb-touch") : executable
# separe et volontairement minimal (donnees de demo statiques, pas
# d'AppController ni de tactile) tant que le design n'est pas valide et que
# le materiel n'est pas en main -- voir simulator/src/main_wide.cpp. Ne
# touche jamais trophy-display-simulator ci-dessus (ecran rond, produit
# actuel en production).
file(GLOB WIDE_FONT_SOURCES ${REPO_ROOT}/src/assets/fonts/*.c)
add_executable(trophy-display-simulator-wide
    src/main_wide.cpp
    src/DisplayDriverSdlWide.cpp
    ${REPO_ROOT}/src/theme/theme.cpp
    ${REPO_ROOT}/src/widgets/widgets.cpp
    ${REPO_ROOT}/src/screens/screens_wide.cpp
    ${REPO_ROOT}/src/assets/lucide_icons.cpp
    ${REPO_ROOT}/src/assets/pocketpsn_logo.cpp
    ${REPO_ROOT}/src/assets/premium_trophies.cpp
    ${REPO_ROOT}/src/assets/stat_icons.cpp
    ${REPO_ROOT}/src/assets/stat_card_background.cpp
    ${REPO_ROOT}/src/assets/stat_glow_dot.cpp
    ${REPO_ROOT}/src/assets/trophy_ring.cpp
    ${REPO_ROOT}/src/assets/trophy_ring_full.cpp
    ${REPO_ROOT}/src/assets/trophy_scene_wide.cpp
    ${WIDE_FONT_SOURCES}
    ${REPO_ROOT}/src/ui/ui_model.cpp
    ${REPO_ROOT}/src/ui/strings.cpp
)

target_include_directories(trophy-display-simulator-wide PRIVATE
    ${REPO_ROOT}/include
    ${REPO_ROOT}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb
    ${SDL2_ROOT}/include/SDL2
)

target_link_libraries(trophy-display-simulator-wide PRIVATE
    lvgl
    ${SDL2_ROOT}/lib/libSDL2.dll.a
    winmm setupapi ole32 oleaut32 imm32 version uuid gdi32 user32
)

add_custom_command(TARGET trophy-display-simulator-wide POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SDL2_ROOT}/bin/SDL2.dll
        $<TARGET_FILE_DIR:trophy-display-simulator-wide>
)

# --- Simulateur "live" du board 7" : contrairement a
# trophy-display-simulator-wide ci-dessus (donnees statiques, verifie
# seulement le rendu visuel), celui-ci fait tourner le VRAI AppController +
# WideUiBridge (src/board7inch/WideUiBridge.cpp, le code reellement utilise
# par le firmware main_7inch.cpp) sur PC, en mode demo -- voir
# simulator/src/main_wide_live.cpp. Verifie mapProfile()/formatUpdated()/
# le defilement automatique/le patch sleepTimeoutSeconds=0 AVANT d'avoir le
# materiel reel en main.
add_executable(trophy-display-simulator-wide-live
    src/main_wide_live.cpp
    src/DisplayDriverSdlWide.cpp
    src/WiFiManagerStub.cpp
    ${REPO_ROOT}/src/board7inch/WideUiBridge.cpp
    ${REPO_ROOT}/src/theme/theme.cpp
    ${REPO_ROOT}/src/widgets/widgets.cpp
    ${REPO_ROOT}/src/screens/screens_wide.cpp
    ${REPO_ROOT}/src/assets/lucide_icons.cpp
    ${REPO_ROOT}/src/assets/pocketpsn_logo.cpp
    ${REPO_ROOT}/src/assets/premium_trophies.cpp
    ${REPO_ROOT}/src/assets/stat_icons.cpp
    ${REPO_ROOT}/src/assets/stat_card_background.cpp
    ${REPO_ROOT}/src/assets/stat_glow_dot.cpp
    ${REPO_ROOT}/src/assets/trophy_ring.cpp
    ${REPO_ROOT}/src/assets/trophy_ring_full.cpp
    ${REPO_ROOT}/src/assets/trophy_scene_wide.cpp
    ${WIDE_FONT_SOURCES}
    ${REPO_ROOT}/src/ui/ui_model.cpp
    ${REPO_ROOT}/src/ui/strings.cpp
    ${DATA_SOURCES}
    ${SHARED_LOGIC_SOURCES}
)

target_include_directories(trophy-display-simulator-wide-live PRIVATE
    ${REPO_ROOT}/include
    ${REPO_ROOT}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ArduinoJson
    ${SDL2_ROOT}/include/SDL2
)

target_link_libraries(trophy-display-simulator-wide-live PRIVATE
    lvgl
    ${SDL2_ROOT}/lib/libSDL2.dll.a
    winmm setupapi ole32 oleaut32 imm32 version uuid gdi32 user32
)

add_custom_command(TARGET trophy-display-simulator-wide-live POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SDL2_ROOT}/bin/SDL2.dll
        $<TARGET_FILE_DIR:trophy-display-simulator-wide-live>
)
