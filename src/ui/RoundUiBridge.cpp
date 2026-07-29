#include "ui/RoundUiBridge.h"

#include <cstdio>

#include "data/PocketPsnProvider.h"
#include "data/ProviderFactory.h"
#include "ui/strings.hpp"
#include "ui/trophy_display_ui.hpp"

namespace {

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

trophy::NetworkState mapNetwork(WifiState state) {
  return state == WifiState::kConnected ? trophy::NetworkState::Online : trophy::NetworkState::Offline;
}

// Traduit une erreur interne (code + message technique, voir
// src/data/SyncStatus.h) en un texte utilisateur -- jamais de JSON brut, de
// code HTTP isole ou de trace technique (voir consignes du design final).
void translateError(const AppError& error, AppLanguage lang, std::string& outTitle,
                     std::string& outMessage) {
  using trophy::Str;
  using trophy::tr;
  switch (error.code) {
    case PocketPsnProvider::kErrorEmptyResponse:
      outTitle = tr(lang, Str::kErrAccountNotFoundTitle);
      outMessage = tr(lang, Str::kErrAccountNotFoundMessage);
      return;
    case PocketPsnProvider::kErrorInvalidJson:
      outTitle = tr(lang, Str::kErrInvalidResponseTitle);
      outMessage = tr(lang, Str::kErrInvalidResponseMessage);
      return;
    case PocketPsnProvider::kErrorHttpBeginFailed:
      outTitle = tr(lang, Str::kErrConnectionFailedTitle);
      outMessage = tr(lang, Str::kErrConnectionFailedMessage);
      return;
    default:
      break;
  }
  if (error.code == -100) {
    outTitle = tr(lang, Str::kErrInconsistentDataTitle);
    outMessage = tr(lang, Str::kErrInconsistentDataMessage);
    return;
  }
  if (error.code >= 400) {
    outTitle = tr(lang, Str::kErrServiceUnavailableTitle);
    outMessage = tr(lang, Str::kErrServiceUnavailableMessage);
    return;
  }
  outTitle = tr(lang, Str::kErrSyncFailedTitle);
  outMessage = tr(lang, Str::kErrSyncFailedMessage);
}

}  // namespace

void RoundUiBridge::begin() { trophy::ui_init(); }

void RoundUiBridge::tick(uint32_t nowMillis) {
  nowMillis_ = nowMillis;
  trophy::ui_app_tick(nowMillis);

  if (syncDwellPending_ && nowMillis_ >= syncDwellDueMs_) {
    syncDwellPending_ = false;
    if (trophy::ui_get_page() == trophy::UiPage::Sync) {
      trophy::ui_show_page(trophy::UiPage::Dashboard);
    }
  }
}

std::string RoundUiBridge::formatUpdated(AppLanguage lang) const {
  using trophy::Str;
  using trophy::tr;
  // Trait ASCII simple (pas le tiret cadratin "—") : aucune des polices
  // td_font_* generees n'inclut U+2014 dans sa plage de caracteres (voir
  // Opts: -r ... dans src/assets/fonts/td_font_12.c) -- LVGL affichait donc
  // un rectangle "glyphe manquant" a la place. Meme bug latent que sur le
  // board 7" (WideUiBridge.cpp), corrige ici aussi par coherence.
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

trophy::ProfileData RoundUiBridge::mapProfile(const AppState& state) const {
  // Ne jamais reinitialiser error_title/error_message/celebration ici :
  // seuls showError()/showTrophyDelta() les font evoluer (voir leurs
  // commentaires) -- App::set_profile() remplace tout le struct d'un coup,
  // donc on part de la valeur actuellement affichee pour ne pas l'ecraser
  // avec une valeur par defaut a chaque tick.
  trophy::ProfileData p = trophy::ui_get_profile();

  p.username = state.profile.username.empty() ? "-" : state.profile.username;
  p.level = state.profile.level;
  p.progress = state.profile.levelProgressPercent;
  p.total = state.stats.totalTrophies;
  p.platinum = state.stats.platinum;
  p.gold = state.stats.gold;
  p.silver = state.stats.silver;
  p.bronze = state.stats.bronze;
  p.games_completed = state.stats.gamesCompleted;
  p.completion = static_cast<int>(state.stats.completionRatePercent + 0.5f);
  p.world_rank =
      state.stats.worldRank > 0 ? ("#" + trophy::format_number(state.stats.worldRank)) : "-";
  p.play_time = state.stats.playtimeHours > 0.0f
                    ? (trophy::format_number(static_cast<int>(state.stats.playtimeHours + 0.5f)) + " h")
                    : "-";
  p.updated = formatUpdated(state.settings.language);
  p.offline_message = trophy::tr(state.settings.language, trophy::Str::kOfflineMessageDefault);
  p.offline = state.sync.isOffline;
  p.sync = mapSyncState(state.sync.state);
  p.reduced_motion = !state.settings.animationsEnabled;
  p.brightness = state.settings.brightnessPercent;
  // Pocket PSN ne fournit jamais de liste detaillee des derniers trophees
  // (uniquement des totaux/statistiques agregees, voir PocketPsnParser.cpp
  // et docs/POCKETPSN_PROTOCOL.md) -- l'ecran Trophees doit donc afficher
  // son etat vide plutot que la vitrine de demonstration (fixture) des
  // qu'un vrai compte Pocket PSN est actif. Meme decision pure que
  // main.cpp au demarrage (voir ProviderFactory.h), recalculee ici : reste
  // coherente sans etat supplementaire a faire transiter par AppState.
  p.trophy_feed_available = !ProviderFactory::shouldUsePocketPsn(state.settings);
  return p;
}

void RoundUiBridge::setAppState(const AppState& state) {
  state_ = state;
  if (state.sync.lastSyncEpoch != 0 && state.sync.lastSyncEpoch != lastSeenSyncEpoch_) {
    lastSeenSyncEpoch_ = state.sync.lastSyncEpoch;
    lastSyncMillis_ = nowMillis_;
    everSynced_ = true;
  }
  if (!state.sync.lastError.hasError()) {
    lastErrorCode_ = 0;
    lastErrorMessage_.clear();
  }

  trophy::ui_set_language(state.settings.language);
  trophy::ui_set_profile(mapProfile(state));
  trophy::ui_set_network_state(mapNetwork(state.network.state));
  trophy::ui_set_animations_enabled(state.settings.animationsEnabled);
  trophy::ui_set_brightness(static_cast<uint8_t>(state.settings.brightnessPercent));
  trophy::ui_set_auto_rotation(state.settings.autoRotateEnabled,
                                static_cast<uint16_t>(state.settings.rotationIntervalSeconds));
}

void RoundUiBridge::showSyncState(SyncState state) {
  trophy::ui_set_sync_state(mapSyncState(state));
  trophy::UiPage current = trophy::ui_get_page();

  // Transitions automatiques (pas un geste utilisateur) : jamais animees --
  // voir RoundUiBridge.h / ui_show_page_immediate() pour la justification
  // (bug reel trouve et corrige le 2026-07-22 : lv_scr_load_anim() reste en
  // cours si un deuxieme changement de page survient avant la fin de
  // l'animation de 220 ms et qu'aucun lv_timer_handler() n'a eu l'occasion
  // de la terminer entre-temps, ce qui corrompt l'ecran ensuite supprime).
  if (isSyncActive(state)) {
    syncDwellPending_ = false;
    if (current != trophy::UiPage::Sync) trophy::ui_show_page_immediate(trophy::UiPage::Sync);
  } else if (state == SyncState::kOffline) {
    syncDwellPending_ = false;
    // Uniquement sur le front montant (premiere fois qu'on observe
    // kOffline, pas a chaque tick tant qu'on y reste) : voir wasOffline_
    // dans RoundUiBridge.h pour le bug reel que ca corrige (bouton "Retour
    // dashboard" qui semblait ne rien faire, ecrase par ce redirect au tick
    // suivant).
    if (!wasOffline_ && current != trophy::UiPage::Offline) {
      trophy::ui_show_page_immediate(trophy::UiPage::Offline);
    }
  } else if (state == SyncState::kSuccess && current == trophy::UiPage::Sync && !syncDwellPending_) {
    syncDwellPending_ = true;
    syncDwellDueMs_ = nowMillis_ + kSyncDwellMs;
  } else if (state == SyncState::kSuccess && current == trophy::UiPage::Offline) {
    // Meme retour automatique que ci-dessus, mais depuis Hors-ligne plutot
    // que Sync : une synchronisation (typiquement une resynchronisation
    // demo/tres rapide, voir DemoDataProvider) peut passer de kOffline a
    // kSuccess sans qu'aucun tick intermediaire n'observe un etat
    // isSyncActive (kConnecting/kDownloading/...), donc sans jamais afficher
    // l'ecran Sync -- sans ce cas, l'ecran restait bloque sur Hors-ligne
    // indefiniment malgre une synchronisation reussie. Bug reel trouve le
    // 2026-07-23 lors de la verification du demarrage reel (Boot B1).
    trophy::ui_show_page_immediate(trophy::UiPage::Dashboard);
  }

  wasOffline_ = (state == SyncState::kOffline);
}

void RoundUiBridge::showTrophyDelta(const TrophyDelta& delta) {
  int kindsHit = (delta.platinumDelta > 0) + (delta.goldDelta > 0) + (delta.silverDelta > 0) + (delta.bronzeDelta > 0);
  trophy::TrophyKind kind = trophy::TrophyKind::Multiple;
  if (kindsHit <= 1) {
    if (delta.platinumDelta > 0) kind = trophy::TrophyKind::Platinum;
    else if (delta.goldDelta > 0) kind = trophy::TrophyKind::Gold;
    else if (delta.silverDelta > 0) kind = trophy::TrophyKind::Silver;
    else kind = trophy::TrophyKind::Bronze;
  }
  syncDwellPending_ = false;
  trophy::ui_show_new_trophy(kind, static_cast<uint32_t>(delta.totalDelta > 0 ? delta.totalDelta : 1));
}

void RoundUiBridge::showError(const AppError& error) {
  if (error.code == lastErrorCode_ && error.message == lastErrorMessage_) return;
  lastErrorCode_ = error.code;
  lastErrorMessage_ = error.message;
  syncDwellPending_ = false;

  std::string title, message;
  translateError(error, state_.settings.language, title, message);
  trophy::ui_show_error(title.c_str(), message.c_str());
}

void RoundUiBridge::swipeLeft() { trophy::ui_swipe_left(); }
void RoundUiBridge::swipeRight() { trophy::ui_swipe_right(); }
void RoundUiBridge::activate() { trophy::ui_activate(); }
void RoundUiBridge::longPress() { trophy::ui_long_press(); }

bool RoundUiBridge::onDashboard() const { return trophy::ui_get_page() == trophy::UiPage::Dashboard; }

void RoundUiBridge::showBootProgress(BootStep step) {
  // Pourcentages et libelles demandes explicitement -- voir BootStep.h pour
  // l'etape reelle exacte que chacun represente. AppController ne connait
  // que l'etape franchie ; le choix du pourcentage/texte affiche est un
  // choix de presentation, propre a cette implementation de UiBridge (meme
  // separation que translateError() ci-dessus pour les erreurs).
  using trophy::Str;
  using trophy::tr;
  const AppLanguage lang = state_.settings.language;
  uint8_t percent = 0;
  const char* status = tr(lang, Str::kBootInitializing);
  switch (step) {
    case BootStep::kSystemStart:
      percent = 0;
      status = tr(lang, Str::kBootSystemStart);
      break;
    case BootStep::kConfigLoaded:
      percent = 15;
      status = tr(lang, Str::kBootConfigLoaded);
      break;
    case BootStep::kCacheLoaded:
      percent = 30;
      status = tr(lang, Str::kBootCacheLoaded);
      break;
    case BootStep::kNetworkInit:
      percent = 45;
      status = tr(lang, Str::kBootNetworkInit);
      break;
    case BootStep::kProfileLoaded:
      percent = 60;
      status = tr(lang, Str::kBootLoadingProfile);
      break;
    case BootStep::kDataReady:
      percent = 75;
      status = tr(lang, Str::kBootDataReady);
      break;
    case BootStep::kUiReady:
      percent = 90;
      status = tr(lang, Str::kBootUiReady);
      break;
    case BootStep::kAppReady:
      percent = 100;
      status = tr(lang, Str::kReady);
      break;
  }
  trophy::ui_boot_set_progress(percent);
  trophy::ui_boot_set_status(status);
  if (step == BootStep::kAppReady) {
    trophy::ui_boot_finish();
  }
}
