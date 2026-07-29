#pragma once

#include <atomic>
#include <cstdint>

#include "data/TrophyDelta.h"
#include "data/TrophyRepository.h"
#include "data/SyncStatus.h"

// Orchestre les synchronisations manuelles/automatiques (voir
// docs/IMPLEMENTATION_PLAN.md). Seule methode appelee par la boucle
// principale : poll() -- jamais bloquante.
//
// Note d'implementation (portabilite/concurrence) : PocketPsnProvider::
// requestRefresh() est un appel HTTPS bloquant (HTTPClient d'Arduino ne
// propose pas d'API non bloquante). Pour respecter "ne jamais bloquer la
// boucle principale", ce travail est execute sur une tache FreeRTOS dediee
// cote firmware ; la synchronisation entre cette tache et la boucle
// principale se limite a un unique drapeau atomique (taskDone_), ce qui
// garantit (via l'ordre release/acquire du C++) que repository_.poll() ne
// lit les resultats qu'apres leur ecriture complete par la tache. Aucune
// autre donnee partagee n'est touchee par la tache de fond : elle appelle
// uniquement TrophyRepository::requestRefresh() (pur relai vers le
// provider, sans mutation de cache/etat) -- voir TrophyRepository.cpp.
// Sur simulateur/desktop, les providers (Demo/Mock) sont deja synchrones et
// instantanes : aucune tache n'est necessaire, le cycle s'execute en ligne.
class SyncService {
 public:
  explicit SyncService(TrophyRepository& repository);
  ~SyncService();

  // syncIntervalMinutes : issu d'AppSettings ; 0 = synchronisation
  // automatique desactivee (uniquement manuelle).
  void configure(int syncIntervalMinutes);

  // Demande une synchronisation manuelle a la prochaine occasion (sans
  // effet si une synchronisation est deja en cours).
  void requestManualSync();

  // Annule une synchronisation en attente (n'interrompt pas brutalement
  // une tache reseau deja en cours -- elle se termine et son resultat est
  // simplement ignore).
  void cancel();

  void setNetworkAvailable(bool available);

  // A appeler periodiquement depuis la boucle principale (jamais
  // bloquant). nowMillis = millis() ; nowEpoch = TimeService::nowEpoch().
  void poll(uint32_t nowMillis, uint32_t nowEpoch);

  const SyncStatus& status() const { return status_; }

  bool consumePendingDelta(TrophyDelta& outDelta) { return repository_.consumePendingDelta(outDelta); }

  static constexpr int kMaxConsecutiveRetries = 5;
  static constexpr uint32_t kBaseBackoffMs = 5000;
  static constexpr uint32_t kMaxBackoffMs = 300000;  // 5 min

  // Delai de stabilisation apres une reconnexion (transition hors-ligne ->
  // connecte) avant de forcer une synchro unique -- voir poll(). Evite de
  // synchroniser sur de simples micro-coupures Wi-Fi (toute nouvelle
  // coupure pendant la fenetre annule la stabilisation et relance
  // l'attente depuis zero a la prochaine reconnexion).
  static constexpr uint32_t kReconnectStabilizationMs = 4000;

 private:
  void startSync(uint32_t nowMillis, uint32_t nowEpoch);
  void finishSync(bool success, uint32_t nowEpoch);
  uint32_t backoffDelayMillis() const;

  TrophyRepository& repository_;
  SyncStatus status_;

  int syncIntervalMinutes_ = 30;
  bool networkAvailable_ = false;
  bool manualSyncRequested_ = false;
  bool cancelRequested_ = false;

  uint32_t lastAttemptMillis_ = 0;
  // false tant qu'aucune synchronisation n'a jamais ete tentee : garantit
  // que la toute premiere synchronisation se declenche des que le reseau
  // est disponible plutot que d'attendre un intervalle complet (nowMillis
  // et lastAttemptMillis_ valent tous deux ~0 au demarrage -- sans ce
  // drapeau, "nowMillis - lastAttemptMillis_" ne depasse syncIntervalMinutes_
  // qu'apres un vrai intervalle complet ecoule depuis le boot ; bug reel
  // rencontre et corrige le 2026-07-15, voir HANDOFF_PROGRESS.md).
  bool everAttempted_ = false;

  // Detection de reconnexion + fenetre de stabilisation (voir poll() et
  // kReconnectStabilizationMs) -- demande explicite de l'utilisateur pour
  // qu'une reconnexion Wi-Fi apres une vraie coupure declenche une seule
  // synchro, sans jamais contourner le backoff (ci-dessus) si des echecs
  // sont deja en cours.
  bool previousNetworkAvailable_ = false;
  bool reconnectStabilizing_ = false;
  uint32_t reconnectStableSinceMillis_ = 0;
  bool reconnectSyncRequested_ = false;

  bool taskRunning_ = false;
  std::atomic<bool> taskDone_{false};

#ifdef ARDUINO
  static void taskEntryPoint(void* arg);
#endif
};
