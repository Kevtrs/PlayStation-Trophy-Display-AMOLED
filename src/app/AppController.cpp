#include "app/AppController.h"

#include "utils/Logger.h"

AppController::AppController(IPersistentStore& configStore, IPersistentStore& cacheStore,
                              TrophyDataProvider& provider, IBrightnessBackend& brightnessBackend,
                              IWiFiManager& wifi, UiBridge& ui)
    : config_(configStore),
      cache_(cacheStore),
      repository_(provider, cache_),
      sync_(repository_),
      power_(brightnessBackend),
      wifi_(wifi),
      ui_(ui) {}

void AppController::begin() {
  // Progression reelle de l'ecran Boot (voir BootStep.h) : chaque etape est
  // signalee juste apres avoir reellement eu lieu, jamais en avance ni
  // simulee par ecoulement du temps. begin() ne couvre que les etapes
  // synchrones (0/15/30/45/60 %) ; les etapes 75/90/100 % dependent de la
  // resolution (reussie ou non) de la synchronisation initiale, forcement
  // asynchrone -- voir tick().
  ui_.showBootProgress(BootStep::kSystemStart);

  // 1. Config, 2. cache hors-ligne -- avant toute tentative reseau (voir
  // sequence de demarrage, docs/IMPLEMENTATION_PLAN.md).
  config_.load();
  Logger::info("[BOOT] Config");
  ui_.showBootProgress(BootStep::kConfigLoaded);
  repository_.loadFromCache();
  Logger::info("[BOOT] Cache: %s", repository_.hasData() ? "trouve" : "absent (premier demarrage)");
  ui_.showBootProgress(BootStep::kCacheLoaded);

  const AppSettings& settings = config_.settings();
  time_.begin(settings.timezone);
  power_.configure(settings.brightnessPercent, settings.sleepTimeoutSeconds);
  sync_.configure(settings.syncIntervalMinutes);
  wifi_.begin(settings.wifiSsid, settings.wifiPassword);
  Logger::info("[BOOT] Network: initialisation demarree");
  ui_.showBootProgress(BootStep::kNetworkInit);

  state_.settings = settings;
  state_.profile = repository_.profile();
  state_.stats = repository_.stats();
  state_.network = wifi_.status();
  state_.sync = sync_.status();
  state_.sync.isDemo = settings.demoMode;
  state_.sync.isOffline = true;  // pas encore de reseau confirme au demarrage
  ui_.showBootProgress(BootStep::kProfileLoaded);

  Logger::info("AppController: demarrage (cache=%s, demoMode=%s)", repository_.hasData() ? "present" : "absent",
               settings.demoMode ? "oui" : "non");

  publishState();
}

void AppController::tick(uint32_t nowMillis) {
  time_.poll();
  power_.poll(nowMillis);
  wifi_.poll(nowMillis);

  state_.network = wifi_.status();
  sync_.setNetworkAvailable(state_.network.state == WifiState::kConnected);

  uint32_t nowEpoch = time_.nowEpoch();
  sync_.poll(nowMillis, nowEpoch);

  TrophyDelta delta;
  if (repository_.consumePendingDelta(delta)) {
    ui_.showTrophyDelta(delta);
  }

  SyncState currentSyncState = sync_.status().state;

  // Suite de la progression Boot (voir begin() et BootStep.h). Deux cas
  // distincts font avancer "recuperation des donnees Pocket PSN" :
  // 1. Reseau reellement absent (WifiState::kAccessPoint -- aucun SSID
  //    enregistre, voir IWiFiManager::begin()) : ne jamais attendre un
  //    reseau qui ne viendra pas, on avance immediatement.
  // 2. Une tentative de synchronisation reelle (reseau configure, en cours
  //    de connexion ou deja connecte) a reellement abouti, en succes ou en
  //    echec.
  // A NE PAS confondre avec SyncState::kOffline seul : SyncService::poll()
  // (voir SyncService.cpp) rapporte kOffline pendant TOUTE la fenetre de
  // connexion Wi-Fi (WifiState::kConnecting, ex. 600 ms en simulateur, voir
  // WiFiManagerStub::kSimulatedConnectDelayMs), pas seulement quand le
  // reseau est genuinement absent -- bug reel trouve le 2026-07-23 : traiter
  // kOffline seul comme "resolu" faisait basculer vers le Dashboard avant
  // meme la fin de la connexion reelle, puis RoundUiBridge::showSyncState()
  // redirigeait aussitot vers Hors-ligne (kOffline encore vrai a cet
  // instant) -- ecran qui ne revient jamais tout seul vers le Dashboard
  // apres un succes ulterieur (ce retour automatique n'existe que depuis
  // l'ecran Sync, voir kSyncDwellMs). bootReady_ suit dans la foulee : notre
  // interface LVGL est deja construite (pas de construction differee a
  // attendre), donc "preparation de l'interface" et "application prete" se
  // succedent reellement, sans delai fabrique pour la forme.
  bool networkGenuinelyAbsent = state_.network.state == WifiState::kAccessPoint;
  bool syncAttemptResolved = currentSyncState == SyncState::kSuccess || currentSyncState == SyncState::kError;
  if (!bootDataResolved_ && (networkGenuinelyAbsent || syncAttemptResolved)) {
    bootDataResolved_ = true;
    Logger::info("[BOOT] PocketPSN: %s",
                  networkGenuinelyAbsent
                      ? "reseau absent, cache/mode demo utilise"
                      : (currentSyncState == SyncState::kSuccess ? "synchronisation reussie" : "echec (voir cache/mode demo)"));
    ui_.showBootProgress(BootStep::kDataReady);
  }
  if (bootDataResolved_ && !bootReady_) {
    bootReady_ = true;
    Logger::info("[BOOT] UI ready");
    ui_.showBootProgress(BootStep::kUiReady);
    ui_.showBootProgress(BootStep::kAppReady);
  }

  if (currentSyncState == SyncState::kSuccess && previousSyncState_ != SyncState::kSuccess) {
    // Une synchronisation reelle vient d'aboutir a l'instant (front
    // montant, pas simplement "l'etat vaut encore kSuccess" -- voir
    // AppController.h) : elle reprend naturellement la main sur toute
    // donnee de debug affichee (voir debugSetDisplayedData()).
    debugOverrideActive_ = false;
  }
  previousSyncState_ = currentSyncState;
  if (!debugOverrideActive_) {
    state_.profile = repository_.profile();
    state_.stats = repository_.stats();
  }
  state_.sync = sync_.status();
  state_.sync.isDemo = config_.settings().demoMode;

  publishState();
}

void AppController::notifyTouchActivity(uint32_t nowMillis) { power_.notifyActivity(nowMillis); }

bool AppController::isDisplayAwake() const { return power_.state() == PowerManager::PowerState::kAwake; }

void AppController::requestManualSync() { sync_.requestManualSync(); }

void AppController::forgetWifiNetwork() {
  config_.mutableSettings().wifiSsid.clear();
  config_.mutableSettings().wifiPassword.clear();
  config_.save();
  wifi_.forgetNetwork();
}

void AppController::requestWifiReconnect() { wifi_.requestReconnect(); }

void AppController::connectToWifi(const std::string& ssid, const std::string& password) {
  AppSettings settings = config_.settings();
  settings.wifiSsid = ssid;
  settings.wifiPassword = password;
  config_.mutableSettings() = settings;
  config_.save();
  applySettingsAndReconfigure(config_.settings());
  publishState();
}

void AppController::factoryReset() {
  config_.resetToDefaults(/*persistNow=*/true);
  cache_.clear();
  repository_.resetInMemoryState();
  wifi_.forgetNetwork();

  state_ = AppState{};
  state_.settings = config_.settings();
  state_.sync.isDemo = state_.settings.demoMode;
  Logger::warn("AppController: reinitialisation complete effectuee (configuration + cache effaces)");
  publishState();
}

void AppController::applySettingsAndReconfigure(const AppSettings& newSettings) {
  power_.configure(newSettings.brightnessPercent, newSettings.sleepTimeoutSeconds);
  sync_.configure(newSettings.syncIntervalMinutes);
  time_.begin(newSettings.timezone);
  wifi_.begin(newSettings.wifiSsid, newSettings.wifiPassword);
  state_.settings = newSettings;
}

bool AppController::applyConfigPatch(const std::string& json, std::string& outError) {
  const std::string previousUsername = config_.settings().psnUsername;
  if (!config_.applyJsonPatch(json, outError)) return false;
  config_.save();
  invalidateCacheIfUsernameChanged(previousUsername);
  applySettingsAndReconfigure(config_.settings());
  publishState();
  return true;
}

void AppController::debugApplySettings(const AppSettings& newSettings) {
  const std::string previousUsername = config_.settings().psnUsername;
  config_.mutableSettings() = newSettings;
  config_.save();
  invalidateCacheIfUsernameChanged(previousUsername);
  applySettingsAndReconfigure(config_.settings());
  publishState();
}

void AppController::invalidateCacheIfUsernameChanged(const std::string& previousUsername) {
  // Un pseudo PSN different invalide toute comparaison avec les anciens
  // chiffres en cache (voir TrophyRepository::validate(), qui rejette toute
  // baisse de trophees par rapport au cache) -- sans ceci, passer du mode
  // demo (chiffres fixes eleves, voir DemoDataProvider) a un vrai compte
  // plus modeste (ou changer de compte PSN) fait rejeter indefiniment la
  // premiere synchronisation reelle ("baisse anormale du nombre de
  // trophees"). Bug reel signale le 2026-07-28 par un utilisateur venant de
  // configurer son premier vrai compte.
  if (config_.settings().psnUsername == previousUsername) return;
  cache_.clear();
  repository_.resetInMemoryState();
  Logger::info("AppController: pseudo PSN modifie, cache de trophees invalide");
}

void AppController::debugSetDisplayedData(const ProfileData& profile, const TrophyStats& stats) {
  debugOverrideActive_ = true;
  state_.profile = profile;
  state_.stats = stats;
  publishState();
}

void AppController::publishState() {
  ui_.setAppState(state_);
  ui_.showSyncState(state_.sync.state);
  if (state_.sync.lastError.hasError()) {
    ui_.showError(state_.sync.lastError);
  }
}
