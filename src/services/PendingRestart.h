#pragma once

#include <cstdint>

// Minuterie portable "repondre puis agir plus tard" (voir
// CaptivePortalServer::handleReboot()/handleReset()) : permet d'envoyer la
// reponse HTTP avant de redemarrer, avec un leger delai pour laisser le
// temps a la reponse de partir sur le reseau. Ne connait rien d'ESP.restart()
// -- c'est a l'appelant (firmware uniquement) d'agir quand consumeDue()
// renvoie true. Purement portable pour rester testable sans materiel.
class PendingRestart {
 public:
  void schedule(uint32_t nowMillis, uint32_t delayMs) {
    pending_ = true;
    triggerAtMillis_ = nowMillis + delayMs;
  }

  // A appeler a chaque tick() : renvoie true une seule fois, au premier
  // appel ou nowMillis a atteint l'echeance programmee.
  bool consumeDue(uint32_t nowMillis) {
    if (pending_ && nowMillis >= triggerAtMillis_) {
      pending_ = false;
      return true;
    }
    return false;
  }

  bool isPending() const { return pending_; }

 private:
  bool pending_ = false;
  uint32_t triggerAtMillis_ = 0;
};
