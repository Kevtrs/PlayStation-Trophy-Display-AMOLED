#pragma once

#include <cstdint>
#include <string>

#include "config/ConfigManager.h"
#include "data/NetworkStatus.h"
#include "data/TrophyRepository.h"
#include "network/IWiFiManager.h"
#include "services/PowerManager.h"
#include "services/SyncService.h"
#include "services/TimeService.h"
#include "storage/TrophyCache.h"
#include "ui/AppState.h"
#include "ui/UiActionListener.h"
#include "ui/UiBridge.h"

// Chef d'orchestre applicatif (voir docs/IMPLEMENTATION_PLAN.md, flux
// AppController -> TrophyRepository -> PocketPsnProvider -> AppState ->
// UiBridge). Seule classe qui connait a la fois la couche donnees/reseau
// et l'UI (via l'interface UiBridge) ; ne depend jamais de LVGL. Le
// materiel (provider reseau, backend de luminosite, stockage, Wi-Fi) est
// injecte par la plateforme (firmware ou simulateur) au moment de la
// construction.
class AppController : public UiActionListener {
 public:
  AppController(IPersistentStore& configStore, IPersistentStore& cacheStore, TrophyDataProvider& provider,
                IBrightnessBackend& brightnessBackend, IWiFiManager& wifi, UiBridge& ui);

  // A appeler une fois au demarrage : charge la config puis le cache
  // hors-ligne (dans cet ordre, avant toute tentative reseau) et publie un
  // premier etat a l'UI -- voir sequence de demarrage,
  // docs/IMPLEMENTATION_PLAN.md.
  void begin();

  // A appeler periodiquement depuis la boucle principale (jamais
  // bloquant). nowMillis = millis()/equivalent portable.
  void tick(uint32_t nowMillis);

  // A appeler a chaque interaction tactile (reveil ecran sans redemarrage).
  void notifyTouchActivity(uint32_t nowMillis);

  // Vrai uniquement si l'ecran est pleinement eveille (ni attenue, ni en
  // veille) -- voir PowerManager::PowerState. A verifier par l'appelant
  // (main.cpp) AVANT de traiter un geste tactile comme une action reelle
  // (sync, navigation...) : le tout premier tap qui reveille l'ecran ne
  // doit jamais AUSSI declencher une action, sinon une synchronisation (ou
  // une navigation) se lance a chaque reveil sans que l'utilisateur l'ait
  // demandee -- bug reel signale le 2026-07-28.
  bool isDisplayAwake() const;

  void requestManualSync();

  // UiActionListener
  void onRequestManualSync() override { requestManualSync(); }

  // Oublie le reseau Wi-Fi configure (efface SSID/mot de passe de la
  // configuration persistante et bascule le WiFiManager en point d'acces
  // de secours).
  void forgetWifiNetwork();

  // Force une nouvelle tentative de connexion Wi-Fi immediate (ignore le
  // backoff courant).
  void requestWifiReconnect();

  // Enregistre et applique de nouveaux identifiants Wi-Fi (ex: recus par
  // POST /api/wifi/connect, voir CaptivePortalServer) : persiste via
  // ConfigManager puis relance la connexion. Contrairement a
  // debugApplySettings(), c'est une action de production normale (pas
  // reservee au debug/simulateur).
  void connectToWifi(const std::string& ssid, const std::string& password);

  // Reinitialisation complete (voir POST /api/reset) : efface la
  // configuration (retour aux valeurs par defaut, y compris Wi-Fi) et le
  // cache de trophees persistant, bascule le Wi-Fi en point d'acces.
  // N'effectue aucun redemarrage elle-meme -- l'appelant (CaptivePortalServer)
  // programme le redemarrage separement, apres avoir repondu au client.
  void factoryReset();

  // Accesseurs en lecture seule pour GET /api/diagnostics (voir
  // src/web/WebApiHandlers.h) : jamais de mot de passe/secret expose.
  bool hasCachedData() const { return repository_.hasData(); }
  uint32_t lastCacheFetchEpoch() const { return repository_.lastFetchEpoch(); }
  int syncTotalCount() const { return state_.sync.totalSyncCount; }
  int syncFailureCount() const { return state_.sync.totalFailureCount; }

  // Applique un patch de configuration (ex: recu par POST /api/config) et
  // reconfigure immediatement les services concernes (luminosite, veille,
  // intervalle de sync) sans redemarrage.
  bool applyConfigPatch(const std::string& json, std::string& outError);

  // --- Debug/simulateur uniquement (voir simulator/DebugPanel.cpp) ---
  // Le panneau de debug ne doit jamais ecrire directement dans
  // UiManager::state() : ces methodes passent par AppController pour que
  // les modifications survivent a tick() (qui republie l'etat a chaque
  // frame) au lieu d'etre ecrasees une frame plus tard.

  // Applique un changement de reglages via ConfigManager (persistant,
  // reconfigure les services concernes), exactement comme applyConfigPatch()
  // mais a partir d'un AppSettings deja construit plutot que d'un JSON.
  void debugApplySettings(const AppSettings& newSettings);

  // Impose un profil/stats affiches sans passer par la validation de
  // TrophyRepository (utile pour tester librement toutes les combinaisons
  // visuelles, y compris des baisses de valeurs qu'une vraie
  // synchronisation rejetterait). Reste actif jusqu'a la prochaine
  // synchronisation reelle reussie, qui reprend naturellement la main
  // (voir tick()).
  void debugSetDisplayedData(const ProfileData& profile, const TrophyStats& stats);

  ConfigManager& config() { return config_; }
  TimeService& time() { return time_; }
  IWiFiManager& wifi() { return wifi_; }
  const AppState& state() const { return state_; }

 private:
  void publishState();
  void applySettingsAndReconfigure(const AppSettings& newSettings);
  void invalidateCacheIfUsernameChanged(const std::string& previousUsername);

  ConfigManager config_;
  TrophyCache cache_;
  TrophyRepository repository_;
  SyncService sync_;
  PowerManager power_;
  TimeService time_;
  IWiFiManager& wifi_;
  UiBridge& ui_;

  AppState state_;
  bool debugOverrideActive_ = false;
  // Detection de front (pas de niveau) sur l'entree en kSuccess : sans
  // cela, un override de debug positionne APRES qu'une synchronisation ait
  // deja reussi serait efface des le tick suivant, puisque l'etat reste a
  // kSuccess bien apres la synchronisation elle-meme (bug reel rencontre et
  // corrige le 2026-07-15, voir HANDOFF_PROGRESS.md).
  SyncState previousSyncState_ = SyncState::kIdle;

  // Progression reelle de l'ecran Boot (voir BootStep.h) : bootDataResolved_
  // passe a vrai la toute premiere fois que la synchronisation initiale
  // aboutit a un etat non transitoire (succes, erreur OU hors ligne -- donc
  // sans jamais attendre indefiniment un reseau absent, voir isSyncActive()
  // dans data/SyncStatus.h), puis bootReady_ marque la fin du tout premier
  // demarrage. Drapeaux a sens unique : ne se declenchent qu'une seule fois
  // par duree de vie du processus, jamais reevalues (un resync/reconnexion/
  // erreur/changement de profil ulterieur ne doit jamais rejouer l'ecran
  // Boot -- voir tick()).
  bool bootDataResolved_ = false;
  bool bootReady_ = false;
};
