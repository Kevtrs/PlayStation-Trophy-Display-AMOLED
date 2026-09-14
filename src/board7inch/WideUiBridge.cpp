#include "board7inch/WideUiBridge.h"

#include <cstdio>

#include "ui/strings.hpp"
#include "utils/Logger.h"

namespace {

// 7s (etait 3s puis 12s) : 12s jugee trop longue a l'usage reel ("trop
// long les ecrans ils reste trop de temps la", retour utilisateur du
// 2026-07-29, apres avoir teste 12s sur le materiel) -- redescendu entre
// l'original (3s, trop rapide) et 12s (trop lent).
constexpr uint32_t kSlideMs = 7000;
constexpr int kSlideCount = 10;

// Meme mapping que RoundUiBridge (src/ui/RoundUiBridge.cpp, non reutilise
// directement : prive a cette classe-la, et couplee au framework de pages
// "App" que screens_wide.cpp n'utilise pas) -- duplique volontairement
// plutot que de toucher au code ecran rond en production.
trophy::SyncState mapSyncState(SyncState state) {
  switch (state) {
    case SyncState::kIdle: return trophy::SyncState::Idle;
    case SyncState::kWaitingForNetwork: return trophy::SyncState::Connecting;
    case SyncState::kConnecting: return trophy::SyncState::Connecting;
    case SyncState::kDownloading: return trophy::SyncState::Fetching;
    case SyncState::kParsing: return trophy::SyncState::Processing;
    case SyncState::kSaving: return trophy::SyncState::Processing;
    case SyncState::kSuccess: return trophy::SyncState::Done;
    case SyncState::kError: return trophy::SyncState::Error;
    case SyncState::kOffline: return trophy::SyncState::Idle;
  }
  return trophy::SyncState::Idle;
}

int syncProgressFor(SyncState state) {
  switch (state) {
    case SyncState::kWaitingForNetwork:
    case SyncState::kConnecting:
      return 24;
    case SyncState::kDownloading:
      return 64;
    case SyncState::kParsing:
    case SyncState::kSaving:
      return 82;
    case SyncState::kSuccess:
      return 100;
    case SyncState::kError:
      return 100;
    case SyncState::kIdle:
    case SyncState::kOffline:
      return 0;
  }
  return 0;
}

}  // namespace

void WideUiBridge::showSyncState(SyncState state) {
  profile_.sync = mapSyncState(state);
  if (!began_ || showingSetup_) {
    lastRawSyncState_ = state;
    return;
  }

  if (isSyncActive(state)) {
    if (syncStateDismissed_ && state == dismissedSyncState_) return;
    if (state == lastRawSyncState_ && showingStateScreen_ && stateScreenKind_ == StateScreenKind::kSync) return;
    lastRawSyncState_ = state;
    showStateScreen(trophy::build_sync_screen_wide(profile_.sync, syncProgressFor(state), language_),
                    StateScreenKind::kSync);
    return;
  }

  syncStateDismissed_ = false;
  dismissedSyncState_ = SyncState::kIdle;
  if (state == lastRawSyncState_ && showingStateScreen_) return;
  lastRawSyncState_ = state;

  if (state == SyncState::kOffline) {
    showStateScreen(trophy::build_offline_screen_wide(profile_, language_), StateScreenKind::kOffline);
    return;
  }
  if (state == SyncState::kSuccess && showingStateScreen_) {
    showStateScreen(trophy::build_sync_screen_wide(trophy::SyncState::Done, 100, language_), StateScreenKind::kSync);
    clearStateScreen();
    lastSlideMs_ = nowMillis_;
    return;
  }
  if (state == SyncState::kIdle && showingStateScreen_) {
    clearStateScreen();
    lastSlideMs_ = nowMillis_;
    loadSlide(slideIndex_);
  }
}

void WideUiBridge::showTrophyDelta(const TrophyDelta& /*delta*/) {
  // Pas d'ecran de celebration dedie cote large pour l'instant.
}

void WideUiBridge::showError(const AppError& error) {
  if (error.hasError()) {
    if (error.code == lastErrorCode_ && error.message == lastErrorMessage_) return;
    lastErrorCode_ = error.code;
    lastErrorMessage_ = error.message;
    Logger::error("[UI 7\"] Erreur applicative (code=%d): %s", error.code, error.message.c_str());
    trophy::ProfileData errorProfile = profile_;
    errorProfile.error_title = language_ == AppLanguage::kFrench ? "Profil indisponible" : "Profile unavailable";
    errorProfile.error_message = error.message.empty()
                                     ? (language_ == AppLanguage::kFrench ? "Synchronisation interrompue."
                                                                          : "Sync interrupted.")
                                     : error.message;
    showStateScreen(trophy::build_error_screen_wide(errorProfile, language_), StateScreenKind::kError);
  }
}

void WideUiBridge::showBootProgress(BootStep step) {
  Logger::info("[UI 7\"] Etape de demarrage: %d", static_cast<int>(step));
}

std::string WideUiBridge::formatUpdated(AppLanguage lang) const {
  using trophy::Str;
  using trophy::tr;
  // Trait ASCII simple (pas le tiret cadratin "—") : aucune des polices
  // td_font_* generees n'inclut U+2014 dans sa plage de caracteres (voir
  // Opts: -r ... dans src/assets/fonts/td_font_12.c) -- LVGL affichait donc
  // un rectangle "glyphe manquant" a la place, visible juste sous le pseudo
  // au tout premier flash reel (avant toute synchronisation). Retour
  // utilisateur du 2026-07-29 (photo du rectangle sur le dashboard).
  if (!everSynced_) return "-";
  uint32_t elapsedMs = nowMillis_ - lastSyncMillis_;
  uint32_t elapsedSec = elapsedMs / 1000;
  if (elapsedSec < 60) return tr(lang, Str::kJustNow);
  char buf[32];
  uint32_t elapsedMin = elapsedSec / 60;
  if (elapsedMin < 60) {
    std::snprintf(buf, sizeof(buf), tr(lang, elapsedMin > 1 ? Str::kMinutesAgoPlural : Str::kMinuteAgoSingular),
                  static_cast<unsigned>(elapsedMin));
    return buf;
  }
  uint32_t elapsedHours = elapsedMin / 60;
  if (elapsedHours < 24) {
    std::snprintf(buf, sizeof(buf), tr(lang, elapsedHours > 1 ? Str::kHoursAgoPlural : Str::kHourAgoSingular),
                  static_cast<unsigned>(elapsedHours));
    return buf;
  }
  uint32_t elapsedDays = elapsedHours / 24;
  std::snprintf(buf, sizeof(buf), tr(lang, elapsedDays > 1 ? Str::kDaysAgoPlural : Str::kDayAgoSingular),
                static_cast<unsigned>(elapsedDays));
  return buf;
}

trophy::ProfileData WideUiBridge::mapProfile(const AppState& state) const {
  trophy::ProfileData p;
  p.username = state.profile.username.empty() ? "-" : state.profile.username;
  p.avatar_file_name = state.profile.avatarFileName;
  p.country = state.profile.country;
  p.level = state.profile.level;
  p.progress = state.profile.levelProgressPercent;
  p.level_remaining_points = state.profile.levelRemainingPoints;
  p.total = state.stats.totalTrophies;
  p.platinum = state.stats.platinum;
  p.gold = state.stats.gold;
  p.silver = state.stats.silver;
  p.bronze = state.stats.bronze;
  p.games_completed = state.stats.gamesCompleted;
  p.completion = static_cast<int>(state.stats.completionRatePercent + 0.5f);
  p.world_rank =
      state.stats.worldRank > 0 ? ("#" + trophy::format_number(state.stats.worldRank)) : "-";
  p.national_rank = state.stats.nationalRank > 0 ? trophy::format_number(state.stats.nationalRank) : "-";
  p.trophy_points = state.stats.trophyPoints > 0 ? trophy::format_number(state.stats.trophyPoints) : "-";
  p.play_time = state.stats.playtimeHours > 0.0f
                    ? (trophy::format_number(static_cast<int>(state.stats.playtimeHours + 0.5f)) + " h")
                    : "-";
  p.updated = formatUpdated(state.settings.language);
  p.offline = state.sync.isOffline;
  p.sync = mapSyncState(state.sync.state);
  p.has_ps_plus = state.profile.hasPsPlus;
  p.brightness = state.settings.brightnessPercent;
  p.reduced_motion = !state.settings.animationsEnabled;
  return p;
}

void WideUiBridge::setAppState(const AppState& state) {
  if (state.sync.lastSyncEpoch != 0 && state.sync.lastSyncEpoch != lastSeenSyncEpoch_) {
    lastSeenSyncEpoch_ = state.sync.lastSyncEpoch;
    lastSyncMillis_ = nowMillis_;
    everSynced_ = true;
  }
  if (!state.sync.lastError.hasError()) {
    lastErrorCode_ = 0;
    lastErrorMessage_.clear();
    if (showingStateScreen_ && stateScreenKind_ == StateScreenKind::kError) {
      clearStateScreen();
    }
  }
  profile_ = mapProfile(state);
  network_ = state.network;
  language_ = state.settings.language;
}

void WideUiBridge::loadSlide(int index) {
  lv_obj_t* next = nullptr;
  switch (index) {
    case 0:
      next = trophy::build_dashboard_screen_wide(profile_, language_);
      break;
    case 1:
      next = trophy::build_trophies_overview_screen_wide(profile_, language_);
      break;
    case 2:
      next = trophy::build_trophy_screen_wide(trophy::TrophyKind::Platinum, profile_.platinum, profile_.total, 0, 4, language_);
      break;
    case 3:
      next = trophy::build_trophy_screen_wide(trophy::TrophyKind::Gold, profile_.gold, profile_.total, 1, 4, language_);
      break;
    case 4:
      next = trophy::build_trophy_screen_wide(trophy::TrophyKind::Silver, profile_.silver, profile_.total, 2, 4, language_);
      break;
    case 5:
      next = trophy::build_trophy_screen_wide(trophy::TrophyKind::Bronze, profile_.bronze, profile_.total, 3, 4, language_);
      break;
    case 6:
      next = trophy::build_statistics_overview_screen_wide(profile_, language_);
      break;
    case 7:
      next = trophy::build_stat_screen_wide(trophy::StatIconKind::Medal, profile_.world_rank.c_str(),
                                             tr(language_, Str::kWorldRank), 2, 4);
      break;
    case 8:
      next = trophy::build_stat_screen_wide(trophy::StatIconKind::Clock, profile_.play_time.c_str(),
                                             tr(language_, Str::kPlayTime), 3, 4);
      break;
    case 9:
      // Mention Pocket PSN (source des donnees) : le board rond a un ecran
      // "A propos" equivalent mais accessible par tactile -- ce board n'en
      // a pas, donc integre au defilement automatique. Retour utilisateur
      // du 2026-07-28 ("zero ref a pocket psn").
      next = trophy::build_credits_screen_wide(language_);
      break;
    default:
      break;
  }
  if (next == nullptr) return;
  showScreen(next);
}

void WideUiBridge::loadWifiSetupScreen() {
  // SSID/IP passes en dur ici plutot que lus depuis network_.ssid/
  // ipAddress : ce sont ceux du point d'acces DE SECOURS que WiFiManager
  // broadcast lui-meme (kApSsid="TrophyDisplay-Setup", voir WiFiManager.cpp)
  // -- network_.ssid/ipAddress refletent le dernier reseau STATION tente,
  // pas l'AP de secours actif.
  showScreen(trophy::build_wifi_setup_screen_wide("TrophyDisplay-Setup", "192.168.4.1", language_));
}

void WideUiBridge::showScreen(lv_obj_t* next) {
  lv_obj_t* previous = currentScreen_;
  lv_scr_load(next);
  currentScreen_ = next;
  if (previous != nullptr) lv_obj_delete(previous);
}

void WideUiBridge::begin() {
  showingSetup_ = network_.state == WifiState::kAccessPoint;
  if (showingSetup_) {
    loadWifiSetupScreen();
  } else {
    slideIndex_ = 0;
    loadSlide(slideIndex_);
  }
  began_ = true;
}

void WideUiBridge::tick(uint32_t nowMillis) {
  nowMillis_ = nowMillis;
  if (!began_) return;

  const bool needsSetup = network_.state == WifiState::kAccessPoint;
  if (needsSetup != showingSetup_) {
    showingSetup_ = needsSetup;
    if (showingSetup_) {
      loadWifiSetupScreen();
    } else {
      // Wi-Fi vient d'etre configure avec succes : reprend le defilement
      // depuis le Dashboard plutot que de continuer un compte a rebours
      // qui tournait dans le vide pendant l'ecran de connexion.
      slideIndex_ = 0;
      lastSlideMs_ = nowMillis;
      clearStateScreen();
      loadSlide(slideIndex_);
    }
    return;
  }
  if (showingSetup_) return;  // defilement en pause tant que la config est requise

  if (nowMillis - lastSlideMs_ >= kSlideMs) {
    lastSlideMs_ = nowMillis;
    slideIndex_ = (slideIndex_ + 1) % kSlideCount;
    loadSlide(slideIndex_);
  }
}
