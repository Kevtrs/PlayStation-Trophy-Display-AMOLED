#include "services/SyncService.h"

#include "utils/Logger.h"

#ifdef ARDUINO
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

SyncService::SyncService(TrophyRepository& repository) : repository_(repository) {}

SyncService::~SyncService() = default;

void SyncService::configure(int syncIntervalMinutes) {
  syncIntervalMinutes_ = syncIntervalMinutes < 0 ? 0 : syncIntervalMinutes;
}

void SyncService::requestManualSync() {
  if (taskRunning_) {
    Logger::warn("SyncService: synchronisation manuelle ignoree, une synchronisation est deja en cours");
    return;
  }
  manualSyncRequested_ = true;
}

void SyncService::cancel() {
  if (taskRunning_) {
    // La tache de fond deja lancee ne peut pas etre interrompue proprement
    // (HTTPClient bloquant) : elle ira a son terme, mais son resultat sera
    // ignore (voir poll()).
    cancelRequested_ = true;
    Logger::info("SyncService: annulation demandee, la requete en cours sera ignoree a sa fin");
  } else {
    manualSyncRequested_ = false;
    status_.state = networkAvailable_ ? SyncState::kIdle : SyncState::kOffline;
  }
}

void SyncService::setNetworkAvailable(bool available) { networkAvailable_ = available; }

uint32_t SyncService::backoffDelayMillis() const {
  if (status_.consecutiveFailures <= 0) return 0;
  int exponent = status_.consecutiveFailures - 1;
  uint64_t delay = static_cast<uint64_t>(kBaseBackoffMs) << exponent;  // backoff exponentiel
  if (delay > kMaxBackoffMs) delay = kMaxBackoffMs;
  return static_cast<uint32_t>(delay);
}

void SyncService::startSync(uint32_t nowMillis, uint32_t nowEpoch) {
  lastAttemptMillis_ = nowMillis;
  cancelRequested_ = false;

  if (!networkAvailable_) {
    status_.state = SyncState::kOffline;
    status_.isOffline = true;
    return;
  }

  everAttempted_ = true;
  status_.state = SyncState::kConnecting;

#ifdef ARDUINO
  taskDone_.store(false, std::memory_order_relaxed);
  taskRunning_ = true;
  BaseType_t created = xTaskCreatePinnedToCore(&SyncService::taskEntryPoint, "pocketpsn_sync", 8192, this, 1,
                                                nullptr, tskNO_AFFINITY);
  if (created != pdPASS) {
    Logger::error("SyncService: echec de creation de la tache de synchronisation");
    taskRunning_ = false;
    status_.state = SyncState::kError;
    status_.lastError.code = -200;
    status_.lastError.message = "Echec de creation de la tache de synchronisation";
    finishSync(false, nowEpoch);
  }
#else
  // Providers synchrones sur simulateur/desktop (Demo/Mock) : pas de tache
  // de fond necessaire, le resultat est immediatement disponible.
  repository_.requestRefresh();
  status_.state = SyncState::kSaving;
  bool success = repository_.poll(nowEpoch) == TrophyDataProvider::Status::kSuccess;
  finishSync(success, nowEpoch);
#endif
}

#ifdef ARDUINO
void SyncService::taskEntryPoint(void* arg) {
  SyncService* self = static_cast<SyncService*>(arg);
  // Pur relai vers le provider (voir TrophyRepository::requestRefresh()) :
  // aucune mutation de cache/AppState ne se produit ici, uniquement l'appel
  // HTTPS bloquant. repository_.poll() (qui, lui, mute l'etat) n'est
  // appele que depuis la boucle principale, apres observation de
  // taskDone_ == true.
  self->repository_.requestRefresh();
  self->taskDone_.store(true, std::memory_order_release);
  vTaskDelete(nullptr);
}
#endif

void SyncService::finishSync(bool success, uint32_t nowEpoch) {
  status_.totalSyncCount++;
  status_.isDemo = false;

  if (success) {
    status_.state = SyncState::kSuccess;
    status_.consecutiveFailures = 0;
    status_.lastError = AppError{};
    status_.lastSyncEpoch = nowEpoch;
    status_.pocketPsnVerified = repository_.isProviderVerified();
  } else {
    status_.state = SyncState::kError;
    status_.consecutiveFailures++;
    status_.totalFailureCount++;
    status_.lastError = repository_.lastError();
  }
}

void SyncService::poll(uint32_t nowMillis, uint32_t nowEpoch) {
#ifdef ARDUINO
  if (taskRunning_) {
    if (!taskDone_.load(std::memory_order_acquire)) {
      return;  // synchronisation en cours sur la tache de fond, rien a faire ce tick
    }
    taskRunning_ = false;
    if (cancelRequested_) {
      // Resultat deliberement ignore : ni repository_.poll() ni
      // finishSync() ne sont appeles, la tache terminee n'a donc aucun
      // effet sur le cache/AppState (voir cancel()).
      cancelRequested_ = false;
      status_.state = networkAvailable_ ? SyncState::kIdle : SyncState::kOffline;
    } else {
      status_.state = SyncState::kSaving;
      bool success = repository_.poll(nowEpoch) == TrophyDataProvider::Status::kSuccess;
      finishSync(success, nowEpoch);
    }
  }
#endif

  // Detection de reconnexion (hors-ligne -> connecte) avec fenetre de
  // stabilisation avant de forcer une synchro unique (voir
  // kReconnectStabilizationMs) : toute nouvelle coupure pendant la fenetre
  // annule la stabilisation en cours, ce qui evite plusieurs synchros sur
  // des reconnexions rapides et rapprochees (le delai repart de zero a
  // chaque nouvelle reconnexion tant que la connexion n'est pas restee
  // stable assez longtemps).
  if (!networkAvailable_) {
    reconnectStabilizing_ = false;
  } else if (!previousNetworkAvailable_) {
    reconnectStabilizing_ = true;
    reconnectStableSinceMillis_ = nowMillis;
  }
  previousNetworkAvailable_ = networkAvailable_;

  if (reconnectStabilizing_ && (nowMillis - reconnectStableSinceMillis_) >= kReconnectStabilizationMs) {
    reconnectStabilizing_ = false;
    // Ne force une tentative que si aucun echec n'est deja en cours : ne
    // contourne jamais le backoff existant (kBaseBackoffMs/kMaxBackoffMs
    // ci-dessus), qui gere deja la reprise dans ce cas -- demande
    // explicite de l'utilisateur.
    if (status_.consecutiveFailures == 0) {
      reconnectSyncRequested_ = true;
    }
  }

  if (!networkAvailable_) {
    status_.state = SyncState::kOffline;
    status_.isOffline = true;
    manualSyncRequested_ = false;
    return;
  }
  status_.isOffline = false;
  if (status_.state == SyncState::kOffline) {
    // Reconnecte : ne doit plus afficher "hors-ligne" une fois isOffline
    // remis a false, meme si aucune synchro n'a encore ete (re)declenchee
    // -- sans quoi l'UI laisserait croire que l'app est toujours
    // hors-ligne. Ne declenche aucune synchro, change uniquement l'etat
    // affiche (voir kIdle, deja utilise ailleurs dans ce sens par
    // cancel()).
    status_.state = SyncState::kIdle;
  }

  bool manualRequested = manualSyncRequested_;
  bool autoIntervalElapsed =
      syncIntervalMinutes_ > 0 &&
      (!everAttempted_ ||
       (nowMillis - lastAttemptMillis_) >= static_cast<uint32_t>(syncIntervalMinutes_) * 60000u);
  bool backoffElapsed = status_.consecutiveFailures > 0 && status_.consecutiveFailures <= kMaxConsecutiveRetries &&
                        (nowMillis - lastAttemptMillis_) >= backoffDelayMillis();
  bool reconnectTriggered = reconnectSyncRequested_;

  if (manualRequested || autoIntervalElapsed || backoffElapsed || reconnectTriggered) {
    manualSyncRequested_ = false;
    reconnectSyncRequested_ = false;
    // Une synchro va avoir lieu pour cette periode de connexion (quelle
    // qu'en soit la cause) : annule toute stabilisation de reconnexion
    // encore en attente pour eviter une synchro redondante quelques
    // secondes plus tard (voir plus haut).
    reconnectStabilizing_ = false;
    startSync(nowMillis, nowEpoch);
  }
}
