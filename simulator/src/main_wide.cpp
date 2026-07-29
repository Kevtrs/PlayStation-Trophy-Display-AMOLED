// Mini-simulateur de prevue pour le board Waveshare ESP32-S3-Touch-LCD-7
// (800x480, rectangulaire) -- voir tache "board-7inch-rgb-touch". Affiche
// en boucle les 3 ecrans valides par l'utilisateur le 2026-07-28 (Dashboard
// dense, Trophees/Statistiques geants un par un) avec les vrais assets
// (theme, polices, trophees premium) mais des donnees de demo statiques :
// pas encore branche a AppController/App::tick() (voir screens_wide.hpp),
// volontairement isole tant que le design n'est pas valide et que le
// materiel n'est pas en main pour la vraie integration.
//
// Aucun tactile : conformement a la demande utilisateur ("meme pas besoin
// du tactile, juste ca defile"), ce mini-simulateur ne lit aucun evenement
// souris/tactile -- seul SDL_QUIT (fermeture de fenetre) est gere.

#define SDL_MAIN_HANDLED  // application console classique, pas de wrapper WinMain
#include <SDL.h>
#include <lvgl.h>

#include <cstdio>
#include <filesystem>
#include <string>
#include <system_error>

#include "DisplayDriverSdlWide.h"
#include "screens/screens_wide.hpp"
#include "theme/theme.hpp"
#include "ui/layout_wide.hpp"

namespace {

constexpr Uint32 kSlideMs = 3000;

trophy::ProfileData demoProfile() {
  trophy::ProfileData p;
  p.username = "Kevin_Trophies";
  p.level = 327;
  p.progress = 72;
  p.total = 4286;
  p.platinum = 58;
  p.gold = 214;
  p.silver = 876;
  p.bronze = 3138;
  p.updated = "à l'instant";
  p.sync = trophy::SyncState::Done;
  return p;
}

struct Slide {
  enum class Kind {
    Dashboard,
    Trophy,
    Stat,
    Credits,
    WifiSetup,
    DashboardV2,
    TrophyV2,
    StatV2,
    TrophyV3,
    StatV3,
    TrophyV4
  } kind;
  trophy::TrophyKind trophyKind = trophy::TrophyKind::Platinum;
  int trophyValue = 0;
  trophy::StatIconKind statIcon = trophy::StatIconKind::Gamepad;
  const char* statValue = nullptr;
  const char* statCaption = nullptr;
  int subIndex = 0;
  AppLanguage lang = AppLanguage::kFrench;
};

lv_obj_t* buildSlide(const Slide& s, const trophy::ProfileData& p) {
  switch (s.kind) {
    case Slide::Kind::Dashboard:
      return trophy::build_dashboard_screen_wide(p, s.lang);
    case Slide::Kind::Trophy:
      return trophy::build_trophy_screen_wide(s.trophyKind, s.trophyValue, s.subIndex, 4, s.lang);
    case Slide::Kind::Stat:
      return trophy::build_stat_screen_wide(s.statIcon, s.statValue, s.statCaption, s.subIndex, 4);
    case Slide::Kind::Credits:
      return trophy::build_credits_screen_wide(s.lang);
    case Slide::Kind::WifiSetup:
      return trophy::build_wifi_setup_screen_wide("TrophyDisplay-Setup", "192.168.4.1", s.lang);
    case Slide::Kind::DashboardV2:
      return trophy::build_dashboard_screen_wide_v2(p, s.lang);
    case Slide::Kind::TrophyV2:
      return trophy::build_trophy_screen_wide_v2(s.trophyKind, s.trophyValue, s.subIndex, 4, s.lang);
    case Slide::Kind::StatV2:
      return trophy::build_stat_screen_wide_v2(s.statIcon, s.statValue, s.statCaption, s.subIndex, 4);
    case Slide::Kind::TrophyV3:
      return trophy::build_trophy_screen_wide_v3(s.trophyKind, s.trophyValue, s.subIndex, 4, s.lang);
    case Slide::Kind::StatV3:
      return trophy::build_stat_screen_wide_v3(s.statIcon, s.statValue, s.statCaption, s.subIndex, 4);
    case Slide::Kind::TrophyV4:
      return trophy::build_trophy_screen_wide_v4(s.trophyKind, s.trophyValue, s.subIndex, 4, s.lang);
  }
  return nullptr;
}

}  // namespace

int main(int argc, char** argv) {
  const trophy::ProfileData profile = demoProfile();

  const Slide slides[] = {
      {Slide::Kind::Dashboard},
      {Slide::Kind::Trophy, trophy::TrophyKind::Platinum, profile.platinum, trophy::StatIconKind::Gamepad, nullptr, nullptr, 0},
      {Slide::Kind::Trophy, trophy::TrophyKind::Gold, profile.gold, trophy::StatIconKind::Gamepad, nullptr, nullptr, 1},
      {Slide::Kind::Trophy, trophy::TrophyKind::Silver, profile.silver, trophy::StatIconKind::Gamepad, nullptr, nullptr, 2},
      {Slide::Kind::Trophy, trophy::TrophyKind::Bronze, profile.bronze, trophy::StatIconKind::Gamepad, nullptr, nullptr, 3},
      {Slide::Kind::Stat, trophy::TrophyKind::Platinum, 0, trophy::StatIconKind::Gamepad, "142", "jeux terminés", 0},
      {Slide::Kind::Stat, trophy::TrophyKind::Platinum, 0, trophy::StatIconKind::Percent, "78 %", "complétion moyenne", 1},
      {Slide::Kind::Stat, trophy::TrophyKind::Platinum, 0, trophy::StatIconKind::Medal, "#12 483", "rang mondial", 2},
      {Slide::Kind::Stat, trophy::TrophyKind::Platinum, 0, trophy::StatIconKind::Clock, "3 426 h", "temps de jeu", 3},
      {Slide::Kind::Credits},
      {Slide::Kind::WifiSetup},
      // Previews EN (verification visuelle de la traduction, voir tache
      // "Version anglaise : interface board 7"") : un echantillon suffit
      // (tous les ecrans partagent le meme systeme tr()/Str), pas besoin
      // de dupliquer les 11 slides.
      {Slide::Kind::Dashboard, trophy::TrophyKind::Platinum, 0, trophy::StatIconKind::Gamepad, nullptr, nullptr, 0,
       AppLanguage::kEnglish},
      {Slide::Kind::Trophy, trophy::TrophyKind::Platinum, profile.platinum, trophy::StatIconKind::Gamepad, nullptr,
       nullptr, 0, AppLanguage::kEnglish},
      {Slide::Kind::WifiSetup, trophy::TrophyKind::Platinum, 0, trophy::StatIconKind::Gamepad, nullptr, nullptr, 0,
       AppLanguage::kEnglish},
      {Slide::Kind::DashboardV2},
      {Slide::Kind::TrophyV2, trophy::TrophyKind::Bronze, profile.bronze, trophy::StatIconKind::Gamepad, nullptr,
       nullptr, 3},
      {Slide::Kind::StatV2, trophy::TrophyKind::Platinum, 0, trophy::StatIconKind::Percent, "78 %",
       "complétion moyenne", 1},
      {Slide::Kind::TrophyV3, trophy::TrophyKind::Bronze, profile.bronze, trophy::StatIconKind::Gamepad, nullptr,
       nullptr, 3},
      {Slide::Kind::StatV3, trophy::TrophyKind::Platinum, 0, trophy::StatIconKind::Percent, "78 %",
       "complétion moyenne", 1},
      {Slide::Kind::TrophyV4, trophy::TrophyKind::Bronze, profile.bronze, trophy::StatIconKind::Gamepad, nullptr,
       nullptr, 3},
  };
  constexpr int kSlideCount = sizeof(slides) / sizeof(slides[0]);

  bool exportOnly = false;
  const char* screenshotDir = "simulator_wide_screenshots";
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--export-screenshots") exportOnly = true;
  }

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::fprintf(stderr, "SDL_Init a echoue: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Window* window = SDL_CreateWindow("Trophy Display -- prevue 7\" (800x480)", SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED, trophy::WIDE_WIDTH, trophy::WIDE_HEIGHT,
                                        SDL_WINDOW_SHOWN);
  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  lv_init();
  DisplayDriverSdlWide::init(renderer, trophy::WIDE_WIDTH, trophy::WIDE_HEIGHT);
  trophy::init_theme();

  if (exportOnly) {
    std::error_code ec;
    std::filesystem::create_directories(screenshotDir, ec);
    const char* names[] = {"dashboard",   "platinum",       "gold",         "silver",   "bronze",
                            "stat_games",  "stat_completion", "stat_rank",   "stat_playtime",
                            "credits",     "wifi_setup",
                            "dashboard_en", "platinum_en", "wifi_setup_en", "dashboard_v2_test",
                            "trophy_v2_test", "stat_v2_test",
                            "trophy_v3_test", "stat_v3_test", "trophy_v4_test"};
    lv_obj_t* previousScreen = nullptr;
    for (int i = 0; i < kSlideCount; ++i) {
      lv_obj_t* screen = buildSlide(slides[i], profile);
      lv_scr_load(screen);
      if (previousScreen) lv_obj_delete(previousScreen);
      previousScreen = screen;
      // 90 frames (~1.4s) : laisse le temps aux animations d'entree
      // (float_in/fade_in, jusqu'a ~860ms sur les ecrans Trophees/Stats) de
      // se terminer avant la capture -- 6 frames (~96ms) capturait un etat
      // encore mi-transparent/decale, ajoute lors du passage aux ecrans
      // animes (halo double + entree animee) le 2026-07-28.
      for (int f = 0; f < 90; ++f) {
        lv_tick_inc(16);
        lv_timer_handler();
        SDL_Delay(16);
      }
      DisplayDriverSdlWide::present(renderer);
      char path[256];
      std::snprintf(path, sizeof(path), "%s/%s.png", screenshotDir, names[i]);
      DisplayDriverSdlWide::saveScreenshotPng(path);
      std::printf("Capture enregistree : %s\n", path);
    }
    SDL_Quit();
    return 0;
  }

  int idx = 0;
  lv_obj_t* current = buildSlide(slides[idx], profile);
  lv_scr_load(current);

  bool running = true;
  Uint32 lastSlideMs = SDL_GetTicks();
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) running = false;
    }

    Uint32 now = SDL_GetTicks();
    if (now - lastSlideMs >= kSlideMs) {
      lastSlideMs = now;
      idx = (idx + 1) % kSlideCount;
      lv_obj_t* next = buildSlide(slides[idx], profile);
      lv_obj_t* previous = current;
      lv_scr_load(next);
      current = next;
      lv_obj_delete(previous);
    }

    lv_tick_inc(16);
    lv_timer_handler();
    DisplayDriverSdlWide::present(renderer);
    SDL_Delay(16);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
