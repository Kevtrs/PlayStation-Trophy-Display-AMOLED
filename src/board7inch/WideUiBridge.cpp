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
constexpr uint32_t kStateScreenMs = 7000;
constexpr int kSlideCount = 8;

// Retour automatique au defilement apres ce delai sans contact. Assez long
// pour lire un ecran tranquillement sans qu'il file, assez court pour
// qu'un appareil laisse tel quel apres un appui distrait redevienne seul
// la vitrine qu'il est censee etre.
constexpr uint32_t kIdleResumeMs = 20000;

// Un swipe peut produire plusieurs evenements de geste consecutifs, et
// chaque navigation reconstruit un ecran entier : sans ce garde-fou, un
// geste un peu appuye saute deux ou trois slides d'un coup.
constexpr uint32_t kNavDebounceMs = 250;

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
      // Mention Pocket PSN (source des donnees) : maintenue dans le
      // defilement meme depuis l'ajout du tactile -- la source des donnees
      // doit se voir sans qu'on ait a la chercher dans un menu. Retour
      // utilisateur du 2026-07-28 ("zero ref a pocket psn").
      next = trophy::build_credits_screen_wide(language_);
      break;
    default:
      break;
  }
  if (next == nullptr) return;
  showScreen(next);
  attachNavLayer(next);
}

void WideUiBridge::attachNavLayer(lv_obj_t* screen) {
  if (screen == nullptr) return;
  lv_obj_add_event_cb(screen, &WideUiBridge::onScreenGesture, LV_EVENT_GESTURE, this);
  navOverlay_ = trophy::attach_wide_nav_overlay(screen, slideIndex_, kSlideCount, &WideUiBridge::onNavAction, this);
  // La couche est reconstruite avec chaque ecran : elle doit repartir dans
  // l'etat courant, pas dans son etat par defaut.
  if (navOverlay_ != nullptr && !interactive_) lv_obj_add_flag(navOverlay_, LV_OBJ_FLAG_HIDDEN);
}

void WideUiBridge::onNavAction(trophy::WideNavAction action, int target, void* owner) {
  auto* self = static_cast<WideUiBridge*>(owner);
  if (self == nullptr) return;
  self->handleNav(action, target);
}

void WideUiBridge::onScreenGesture(lv_event_t* e) {
  auto* self = static_cast<WideUiBridge*>(lv_event_get_user_data(e));
  if (self == nullptr) return;
  lv_indev_t* indev = lv_indev_active();
  if (indev == nullptr) return;
  const lv_dir_t dir = lv_indev_get_gesture_dir(indev);
  if (dir == LV_DIR_LEFT) {
    self->handleNav(trophy::WideNavAction::kNext, 0);
  } else if (dir == LV_DIR_RIGHT) {
    self->handleNav(trophy::WideNavAction::kPrev, 0);
  }
}

void WideUiBridge::handleNav(trophy::WideNavAction action, int target) {
  // Appele depuis un callback LVGL : on n'y touche pas a l'arbre d'objets,
  // on note seulement quoi faire (voir navPending_).
  //
  // Toujours avant l'anti-rebond : meme un geste ignore prouve que
  // quelqu'un est devant l'appareil, et doit repousser le retour ambiant.
  noteTouchActivity();
  if (nowMillis_ - lastNavMs_ < kNavDebounceMs) return;
  lastNavMs_ = nowMillis_;
  navPending_ = true;
  pendingAction_ = action;
  pendingTarget_ = target;
}

void WideUiBridge::applyPendingNav() {
  if (!navPending_) return;
  navPending_ = false;
  const trophy::WideNavAction action = pendingAction_;
  const int target = pendingTarget_;

  if (showingSettings_) {
    // Depuis les Reglages, seul le retour a un sens. Le swipe vers la
    // droite ferme aussi, c'est le geste "retour" attendu par reflexe.
    if (action == trophy::WideNavAction::kCloseSettings || action == trophy::WideNavAction::kPrev) closeSettings();
    return;
  }

  switch (action) {
    case trophy::WideNavAction::kPrev:
      slideIndex_ = (slideIndex_ + kSlideCount - 1) % kSlideCount;
      loadSlide(slideIndex_);
      break;
    case trophy::WideNavAction::kNext:
      slideIndex_ = (slideIndex_ + 1) % kSlideCount;
      loadSlide(slideIndex_);
      break;
    case trophy::WideNavAction::kGoTo:
      if (target < 0 || target >= kSlideCount || target == slideIndex_) return;
      slideIndex_ = target;
      loadSlide(slideIndex_);
      break;
    case trophy::WideNavAction::kOpenSettings:
      openSettings();
      break;
    case trophy::WideNavAction::kCloseSettings:
      break;
  }
}

void WideUiBridge::noteTouchActivity() {
  lastTouchMs_ = nowMillis_;
  if (!interactive_) setInteractive(true);
}

void WideUiBridge::pollTouchActivity() {
  for (lv_indev_t* indev = lv_indev_get_next(nullptr); indev != nullptr; indev = lv_indev_get_next(indev)) {
    if (lv_indev_get_type(indev) != LV_INDEV_TYPE_POINTER) continue;
    if (lv_indev_get_state(indev) == LV_INDEV_STATE_PRESSED) {
      noteTouchActivity();
      return;
    }
  }
}

void WideUiBridge::setInteractive(bool interactive) {
  if (interactive_ != interactive) {
    Logger::info("[UI 7\"] Mode %s (t=%ums, slide=%d)", interactive ? "interactif" : "ambiant",
                 static_cast<unsigned>(nowMillis_), slideIndex_);
  }
  interactive_ = interactive;
  if (navOverlay_ == nullptr) return;
  if (interactive) {
    lv_obj_clear_flag(navOverlay_, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(navOverlay_, LV_OBJ_FLAG_HIDDEN);
  }
}

void WideUiBridge::openSettings() {
  if (showingSettings_) return;
  lv_obj_t* next = trophy::build_settings_screen_wide(profile_, language_);
  if (next == nullptr) return;
  showingSettings_ = true;
  showScreen(next);
  lv_obj_add_event_cb(next, &WideUiBridge::onScreenGesture, LV_EVENT_GESTURE, this);
  // Le retour reste visible en permanence sur cet ecran : on y arrive
  // forcement par un appui, le masquer enfermerait l'utilisateur.
  navOverlay_ = trophy::attach_wide_back_overlay(next, &WideUiBridge::onNavAction, this, language_);
}

void WideUiBridge::closeSettings() {
  if (!showingSettings_) return;
  showingSettings_ = false;
  lastSlideMs_ = nowMillis_;
  loadSlide(slideIndex_);
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
  // La couche de navigation est un enfant de l'ecran sortant : elle part
  // avec lui, il ne reste qu'a oublier le pointeur avant la destruction.
  navOverlay_ = nullptr;
  lv_scr_load(next);
  currentScreen_ = next;
  if (previous != nullptr) lv_obj_delete(previous);
}

void WideUiBridge::showStateScreen(lv_obj_t* next, StateScreenKind kind) {
  if (next == nullptr) return;
  showingStateScreen_ = true;
  stateScreenKind_ = kind;
  lastSlideMs_ = nowMillis_;
  // Un ecran d'etat (sync/hors ligne/erreur) est modal : il reprend la
  // main sur la navigation manuelle plutot que de la laisser a moitie
  // active sur un ecran qui n'a ni points ni fleches.
  showingSettings_ = false;
  interactive_ = false;
  showScreen(next);
}

void WideUiBridge::clearStateScreen() {
  showingStateScreen_ = false;
  stateScreenKind_ = StateScreenKind::kNone;
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
  if (showingStateScreen_) {
    const bool canAutoDismiss =
        stateScreenKind_ == StateScreenKind::kSync || stateScreenKind_ == StateScreenKind::kError ||
        stateScreenKind_ == StateScreenKind::kOffline;
    if (canAutoDismiss && nowMillis - lastSlideMs_ >= kStateScreenMs) {
      if (stateScreenKind_ == StateScreenKind::kSync) {
        syncStateDismissed_ = true;
        dismissedSyncState_ = lastRawSyncState_;
      }
      clearStateScreen();
      lastSlideMs_ = nowMillis;
      loadSlide(slideIndex_);
    }
    return;
  }

  // A partir d'ici on est sur un ecran du defilement : c'est le seul
  // contexte ou le tactile de navigation a un sens.
  pollTouchActivity();
  // LVGL est au repos a cet instant : c'est le seul endroit sur pour
  // detruire et reconstruire un ecran.
  applyPendingNav();

  if (interactive_) {
    if (nowMillis - lastTouchMs_ < kIdleResumeMs) return;  // quelqu'un a la main
    // Plus de contact depuis un moment : on referme la navigation et
    // l'appareil redevient la vitrine autonome qu'il doit etre par defaut.
    if (showingSettings_) closeSettings();
    setInteractive(false);
    lastSlideMs_ = nowMillis;
    return;
  }

  if (nowMillis - lastSlideMs_ >= kSlideMs) {
    lastSlideMs_ = nowMillis;
    slideIndex_ = (slideIndex_ + 1) % kSlideCount;
    loadSlide(slideIndex_);
  }
}
