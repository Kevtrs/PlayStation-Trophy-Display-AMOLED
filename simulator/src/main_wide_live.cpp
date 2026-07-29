// Simulateur "live" du board 7" : contrairement a main_wide.cpp (donnees
// statiques figees, uniquement pour valider le rendu visuel des ecrans),
// celui-ci fait tourner le VRAI pipeline applicatif (AppController +
// WideUiBridge, exactement le code utilise par src/main_7inch.cpp) sur PC.
// Objectif : verifier que WideUiBridge::mapProfile()/formatUpdated()/
// loadSlide()/tick() se comportent correctement AVANT d'avoir le materiel
// reel en main -- ce que main_wide.cpp ne pouvait pas prouver (il
// contournait entierement AppController/WideUiBridge).
//
// Mode demo (DemoDataProvider) : c'est exactement le chemin emprunte par
// le vrai board 7" a son tout premier demarrage, tant qu'aucun compte
// Pocket PSN n'est configure (voir ProviderFactory::shouldUsePocketPsn).

#define SDL_MAIN_HANDLED  // application console classique, pas de wrapper WinMain
#include <SDL.h>
#include <lvgl.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>

#include "DisplayDriverSdlWide.h"
#include "NullBrightnessBackend.h"
#include "WiFiManagerStub.h"
#include "app/AppController.h"
#include "board7inch/WideUiBridge.h"
#include "data/DemoDataProvider.h"
#include "storage/FilePersistentStore.h"
#include "theme/theme.hpp"
#include "ui/layout_wide.hpp"

int main(int argc, char** argv) {
  bool exportOnly = false;
  int exportAfterMs = 15000;  // assez pour depasser plusieurs cycles de 3s
  std::string exportPath = "simulator_wide_screenshots/live_after_cycle.png";
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--export-screenshot") exportOnly = true;
    // Permet de capturer un slide precis (voir kSlideMs/loadSlide() dans
    // WideUiBridge.cpp) pour generer les captures MakerWorld sans devoir
    // relancer le cycle complet a chaque fois.
    if (std::string(argv[i]) == "--export-at-ms" && i + 1 < argc) exportAfterMs = std::atoi(argv[++i]);
    if (std::string(argv[i]) == "--export-path" && i + 1 < argc) exportPath = argv[++i];
  }

  const std::string dataDir = "simulator/.simulator_data/wide_live";
  std::error_code ec;
  std::filesystem::remove_all(dataDir, ec);  // depart propre a chaque lancement

  FilePersistentStore configStore(dataDir);
  FilePersistentStore cacheStore(dataDir);
  DemoDataProvider demoProvider;
  WiFiManagerStub wifiStub;
  NullBrightnessBackend brightness;
  WideUiBridge wideUi;

  AppController appController(configStore, cacheStore, demoProvider, brightness, wifiStub, wideUi);

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::fprintf(stderr, "SDL_Init a echoue: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Window* window =
      SDL_CreateWindow("Trophy Display -- 7\" LIVE (vrai AppController)", SDL_WINDOWPOS_UNDEFINED,
                        SDL_WINDOWPOS_UNDEFINED, trophy::WIDE_WIDTH, trophy::WIDE_HEIGHT, SDL_WINDOW_SHOWN);
  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  lv_init();
  DisplayDriverSdlWide::init(renderer, trophy::WIDE_WIDTH, trophy::WIDE_HEIGHT);
  trophy::init_theme();

  // Reproduit exactement setup() de main_7inch.cpp (moins l'init RGB/
  // expander, remplacee par SDL) : appController.begin() PUIS
  // applyConfigPatch(sleepTimeoutSeconds=0) PUIS wideUi.begin().
  appController.begin();
  std::string patchError;
  bool patchOk = appController.applyConfigPatch(R"({"sleepTimeoutSeconds":0})", patchError);
  std::printf("[live] applyConfigPatch(sleepTimeoutSeconds=0): %s (%s)\n", patchOk ? "OK" : "ECHEC",
              patchError.c_str());
  wideUi.begin();

  // Simule une connexion Wi-Fi (comme le scenario 1 de --selftest, voir
  // simulator/src/main.cpp) : sans ceci, SyncService::networkAvailable_
  // reste faux indefiniment (WiFiManagerStub reste en point d'acces par
  // defaut) et AUCUN provider (Demo ou Pocket PSN) n'est jamais interroge
  // -- etat reellement observe au premier lancement de ce simulateur
  // (profil vide, hors-ligne en permanence) : c'est exactement ce qui se
  // passera sur le vrai board 7" tant que le Wi-Fi n'est pas configure via
  // le portail captif. Simule ici la connexion pour verifier le chemin
  // "donnees demo reellement affichees" egalement, avant le materiel.
  wifiStub.simulateConnected("Simulateur");
  std::printf("[live] Demarrage : profil=%s trophees=%d hors-ligne=%s\n",
              appController.state().profile.username.c_str(), appController.state().stats.totalTrophies,
              appController.state().sync.isOffline ? "oui" : "non");

  bool running = true;
  Uint32 startMs = SDL_GetTicks();
  Uint32 lastLogMs = 0;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) running = false;
    }

    Uint32 nowMillis = SDL_GetTicks();

    appController.tick(nowMillis);
    wideUi.tick(nowMillis);

    // Journalise chaque seconde la valeur de luminosite reellement
    // appliquee (NullBrightnessBackend) : verifie concretement que
    // sleepTimeoutSeconds=0 empeche bien tout assombrissement automatique
    // dans le temps (pas juste au demarrage) -- exactement le risque
    // identifie pour un ecran d'ambiance sans tactile.
    if (nowMillis - lastLogMs >= 1000) {
      lastLogMs = nowMillis;
      std::printf("[live] t=%ums luminosite appliquee=%d%% sync=%d maj=\"%s\"\n", nowMillis,
                  brightness.lastAppliedPercent(), static_cast<int>(appController.state().sync.state),
                  "voir ecran");
    }

    lv_tick_inc(16);
    lv_timer_handler();
    DisplayDriverSdlWide::present(renderer);

    if (exportOnly && static_cast<int>(nowMillis - startMs) >= exportAfterMs) {
      std::error_code mkEc;
      std::filesystem::path outPath(exportPath);
      std::filesystem::create_directories(outPath.parent_path(), mkEc);
      DisplayDriverSdlWide::saveScreenshotPng(exportPath.c_str());
      std::printf("[live] Capture enregistree apres %dms : %s\n", exportAfterMs, exportPath.c_str());
      running = false;
    }

    SDL_Delay(16);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
