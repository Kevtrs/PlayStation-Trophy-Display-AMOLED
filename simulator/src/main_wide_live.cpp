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
#include <vector>
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

namespace {

trophy::ProfileData referenceProfile() {
  trophy::ProfileData p;
  p.username = "gaz91610";
  p.avatar_file_name = "A2117_l.png";
  p.country = "FR";
  p.level = 314;
  p.progress = 38;
  p.level_remaining_points = 555;
  p.total = 3376;
  p.platinum = 1;
  p.gold = 185;
  p.silver = 539;
  p.bronze = 2651;
  p.games_completed = 1;
  p.completion = 22;
  p.world_rank = "291 264";
  p.national_rank = "13 717";
  p.trophy_points = "72 885";
  p.play_time = "2 582 h";
  p.updated = "à l'instant";
  p.offline_message = "Le profil reste lisible avec la dernière sauvegarde.";
  p.error_title = "Profil indisponible";
  p.error_message = "La réponse Pocket PSN n'est pas exploitable.";
  p.has_ps_plus = true;
  p.brightness = 82;
  return p;
}

lv_obj_t* buildForcedScreen(const std::string& screenName) {
  trophy::ProfileData p = referenceProfile();
  if (screenName == "dashboard") return trophy::build_dashboard_screen_wide(p, AppLanguage::kFrench);
  if (screenName == "trophies") return trophy::build_trophies_overview_screen_wide(p, AppLanguage::kFrench);
  if (screenName == "platinum") return trophy::build_trophy_screen_wide(trophy::TrophyKind::Platinum, p.platinum, p.total, 0, 4, AppLanguage::kFrench);
  if (screenName == "gold") return trophy::build_trophy_screen_wide(trophy::TrophyKind::Gold, p.gold, p.total, 1, 4, AppLanguage::kFrench);
  if (screenName == "silver") return trophy::build_trophy_screen_wide(trophy::TrophyKind::Silver, p.silver, p.total, 2, 4, AppLanguage::kFrench);
  if (screenName == "bronze") return trophy::build_trophy_screen_wide(trophy::TrophyKind::Bronze, p.bronze, p.total, 3, 4, AppLanguage::kFrench);
  if (screenName == "statistics") return trophy::build_statistics_overview_screen_wide(p, AppLanguage::kFrench);
  if (screenName == "sync") return trophy::build_sync_screen_wide(trophy::SyncState::Fetching, 64, AppLanguage::kFrench);
  if (screenName == "offline") return trophy::build_offline_screen_wide(p, AppLanguage::kFrench);
  if (screenName == "error") return trophy::build_error_screen_wide(p, AppLanguage::kFrench);
  if (screenName == "settings") return trophy::build_settings_screen_wide(p, AppLanguage::kFrench);
  if (screenName == "about") return trophy::build_credits_screen_wide(AppLanguage::kFrench);
  // Deux cas de celebration : un seul palier (le plus courant) et
  // plusieurs paliers d'un coup (affiche le detail par palier).
  if (screenName == "celebration") {
    trophy::TrophyStats gained;
    gained.total = 1;
    gained.platinum = 1;
    return trophy::build_celebration_screen_wide(gained, AppLanguage::kFrench);
  }
  if (screenName == "celebration-multi") {
    trophy::TrophyStats gained;
    gained.total = 7;
    gained.gold = 1;
    gained.silver = 2;
    gained.bronze = 4;
    return trophy::build_celebration_screen_wide(gained, AppLanguage::kFrench);
  }
  // Valeurs du point d'acces de secours reellement diffuse par WiFiManager
  // (kApSsid + softAPIP), pas celles du dernier reseau tente.
  if (screenName == "wifi") {
    return trophy::build_wifi_setup_screen_wide("TrophyDisplay-Setup", "192.168.4.1", AppLanguage::kFrench);
  }
  return nullptr;
}

}  // namespace

// Geste scripte (voir --tap / --swipe) : appui maintenu pendant
// durationMs en interpolant de (x1,y1) vers (x2,y2), puis relache. Un tap
// est le cas degenere ou les deux points sont confondus. La duree doit
// rester bien superieure au pas de scrutation de LVGL, sinon l'appui peut
// passer entierement entre deux lectures du pointeur.
struct ScriptedGesture {
  int x1;
  int y1;
  int x2;
  int y2;
  Uint32 atMs;
  Uint32 durationMs;
  bool done = false;
};
constexpr Uint32 kTapHoldMs = 120;
constexpr Uint32 kSwipeDurationMs = 220;

int main(int argc, char** argv) {
  std::vector<ScriptedGesture> taps;
  bool exportOnly = false;
  int exportAfterMs = 15000;  // assez pour depasser plusieurs cycles de 3s
  std::string exportPath = "simulator_wide_screenshots/live_after_cycle.png";
  std::string forcedScreen;
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--export-screenshot") exportOnly = true;
    // Permet de capturer un slide precis (voir kSlideMs/loadSlide() dans
    // WideUiBridge.cpp) pour generer les captures MakerWorld sans devoir
    // relancer le cycle complet a chaque fois.
    if (std::string(argv[i]) == "--export-at-ms" && i + 1 < argc) exportAfterMs = std::atoi(argv[++i]);
    if (std::string(argv[i]) == "--export-path" && i + 1 < argc) exportPath = argv[++i];
    if (std::string(argv[i]) == "--screen" && i + 1 < argc) forcedScreen = argv[++i];
    // --tap X,Y@MS : rejoue un appui a la position X,Y, MS millisecondes
    // apres le demarrage. Permet de verifier la couche tactile (pause du
    // defilement, fleches, points, engrenage) de facon reproductible et
    // scriptable, sans carte et sans cliquer a la main.
    if (std::string(argv[i]) == "--tap" && i + 1 < argc) {
      int x = 0;
      int y = 0;
      int atMs = 0;
      if (std::sscanf(argv[++i], "%d,%d@%d", &x, &y, &atMs) == 3) {
        taps.push_back({x, y, x, y, static_cast<Uint32>(atMs), kTapHoldMs});
      }
    }
    // --swipe X1,Y1:X2,Y2@MS : rejoue un balayage, pour verifier la
    // navigation au geste (LV_EVENT_GESTURE) sans carte.
    if (std::string(argv[i]) == "--swipe" && i + 1 < argc) {
      int x1 = 0;
      int y1 = 0;
      int x2 = 0;
      int y2 = 0;
      int atMs = 0;
      if (std::sscanf(argv[++i], "%d,%d:%d,%d@%d", &x1, &y1, &x2, &y2, &atMs) == 5) {
        taps.push_back({x1, y1, x2, y2, static_cast<Uint32>(atMs), kSwipeDurationMs});
      }
    }
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
  // La souris tient lieu de doigt : le GT911 du vrai board est enregistre
  // de la meme facon cote firmware (LV_INDEV_TYPE_POINTER, voir
  // main_7inch.cpp), la couche tactile se teste donc ici a l'identique.
  DisplayDriverSdlWide::registerPointer();
  trophy::init_theme();

  if (!forcedScreen.empty()) {
    lv_obj_t* forced = buildForcedScreen(forcedScreen);
    if (forced == nullptr) {
      std::fprintf(stderr, "[live] Ecran inconnu: %s\n", forcedScreen.c_str());
      SDL_DestroyRenderer(renderer);
      SDL_DestroyWindow(window);
      SDL_Quit();
      return 2;
    }
    lv_scr_load(forced);
    for (int i = 0; i < 6; ++i) {
      lv_tick_inc(16);
      lv_timer_handler();
      DisplayDriverSdlWide::present(renderer);
      SDL_Delay(16);
    }
    std::error_code mkEc;
    std::filesystem::path outPath(exportPath);
    std::filesystem::create_directories(outPath.parent_path(), mkEc);
    DisplayDriverSdlWide::saveScreenshotPng(exportPath.c_str());
    std::printf("[live] Capture ecran %s : %s\n", forcedScreen.c_str(), exportPath.c_str());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
  }

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
      // Des appuis scriptes sont en cours : la vraie souris est ignoree.
      // Les deux alimentent le meme pointeur LVGL, et un simple survol de
      // la fenetre pendant un scenario le corrompait (position deplacee au
      // milieu d'un appui = geste parasite). Un test doit donner le meme
      // resultat que la souris soit posee sur la fenetre ou non.
      if (!taps.empty()) continue;
      if (event.type == SDL_MOUSEMOTION) {
        DisplayDriverSdlWide::setPointerState(event.motion.x, event.motion.y,
                                              (event.motion.state & SDL_BUTTON_LMASK) != 0);
      }
      if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        DisplayDriverSdlWide::setPointerState(event.button.x, event.button.y, true);
      }
      if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        DisplayDriverSdlWide::setPointerState(event.button.x, event.button.y, false);
      }
    }

    Uint32 nowMillis = SDL_GetTicks();

    // Gestes scriptes : appui interpole de (x1,y1) vers (x2,y2) puis
    // relache, pour produire un vrai couple presse/relache cote LVGL.
    for (auto& g : taps) {
      if (g.done) continue;
      const Uint32 elapsed = nowMillis - startMs;
      if (elapsed < g.atMs) continue;
      const Uint32 into = elapsed - g.atMs;
      if (into >= g.durationMs) {
        DisplayDriverSdlWide::setPointerState(g.x2, g.y2, false);
        g.done = true;
        std::printf("[live] geste relache a %d,%d (t=%ums)\n", g.x2, g.y2, elapsed);
      } else {
        const float t = static_cast<float>(into) / static_cast<float>(g.durationMs);
        const int x = g.x1 + static_cast<int>((g.x2 - g.x1) * t);
        const int y = g.y1 + static_cast<int>((g.y2 - g.y1) * t);
        DisplayDriverSdlWide::setPointerState(x, y, true);
      }
    }

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
