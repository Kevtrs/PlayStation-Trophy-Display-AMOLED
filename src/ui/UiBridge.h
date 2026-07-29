#pragma once

#include "data/SyncStatus.h"
#include "data/TrophyDelta.h"
#include "ui/AppState.h"
#include "ui/BootStep.h"

// Seule API stable entre la logique applicative (AppController) et
// l'interface (voir docs/IMPLEMENTATION_PLAN.md). Le futur design
// devra fournir une implementation de cette interface (ou une classe la
// contenant, comme UiManager aujourd'hui) sans avoir a toucher a
// AppController/TrophyRepository/SyncService. L'UI ne doit jamais appeler
// HTTP/NVS/WiFi/LittleFS directement -- uniquement recevoir des donnees via
// ces methodes.
class UiBridge {
 public:
  virtual ~UiBridge() = default;

  // Etat complet courant (profil, stats, sync, reseau, reglages) -- appele
  // par AppController a chaque tick.
  virtual void setAppState(const AppState& state) = 0;

  // Notifie un changement d'etat de synchronisation (permet a l'UI de
  // reagir immediatement, ex: afficher l'ecran de sync, sans attendre le
  // prochain setAppState()).
  virtual void showSyncState(SyncState state) = 0;

  // Un ou plusieurs nouveaux trophees valides ont ete detectes (voir
  // TrophyRepository::consumePendingDelta) -- jamais rejoue au redemarrage.
  virtual void showTrophyDelta(const TrophyDelta& delta) = 0;

  // Erreur structuree a afficher (reseau, parsing, config...).
  virtual void showError(const AppError& error) = 0;

  // Etape reelle de demarrage atteinte (voir BootStep.h) -- appele
  // uniquement par AppController::begin()/tick() lors du tout premier
  // demarrage, jamais rejoue. A l'implementation de choisir le pourcentage/
  // texte affiches.
  virtual void showBootProgress(BootStep step) = 0;
};
