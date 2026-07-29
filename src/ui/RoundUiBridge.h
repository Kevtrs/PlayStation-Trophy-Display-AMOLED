#pragma once

#include <cstdint>
#include <string>

#include "ui/UiBridge.h"
#include "ui/ui_model.hpp"

// Implementation de UiBridge branchee sur le design final (voir
// src/ui/trophy_display_ui.hpp). Traduit l'etat applicatif reel
// (AppState/SyncStatus/NetworkStatus/TrophyDelta/AppError, voir
// src/ui/AppState.h) vers le modele de donnees du design
// (trophy::ProfileData/TrophyStats/SyncState/NetworkState) et vers ses
// evenements de navigation (ui_show_page/ui_show_error/ui_show_new_trophy).
//
// Aucun ecran/widget/asset Lucide n'est connu ici -- uniquement l'API
// publique de src/ui/trophy_display_ui.hpp, conformement a la contrainte du
// design ("le projet fonctionnel ne doit pas acceder directement aux
// objets LVGL internes").
class RoundUiBridge : public UiBridge {
 public:
  // A appeler une fois au demarrage, apres AppController::begin() (le
  // premier ecran affiche est Boot, voir trophy::ui_init()).
  void begin();

  // A appeler a chaque tick de la boucle principale (jamais lv_tick_inc/
  // lv_timer_handler ici : le projet cible les pilote deja lui-meme, voir
  // ui_app_tick()). Gere aussi le retour automatique au Dashboard apres une
  // synchronisation reussie (le design ne le fait pas lui-meme, contrairement a
  // l'ancien UiManager -- voir HANDOFF_PROGRESS.md).
  void tick(uint32_t nowMillis);

  // UiBridge
  void setAppState(const AppState& state) override;
  void showSyncState(SyncState state) override;
  void showTrophyDelta(const TrophyDelta& delta) override;
  void showError(const AppError& error) override;
  void showBootProgress(BootStep step) override;

  // Passerelle tactile (voir trophy::ui_swipe_left() etc.) : appelee par le
  // pilote tactile reel (firmware) ou la souris (simulateur), jamais par
  // AppController.
  void swipeLeft();
  void swipeRight();
  void activate();
  void longPress();

  // Utilise par l'appelant (main.cpp) pour decider si un tap doit
  // declencher une vraie synchronisation manuelle (AppController) plutot
  // que le comportement de demonstration interne au design
  // (App::activate() sur Dashboard appelle simulate_sync(), qui ne fait que
  // basculer l'etat affiche vers "Fetching" sans jamais toucher au reseau
  // reel -- inoffensif mais insuffisant pour un vrai bouton produit).
  bool onDashboard() const;

  // Dernier AppState recu via setAppState() -- utilise par --selftest pour
  // verifier que l'etat applicatif est bien propage jusqu'a l'UI (meme role
  // que MinimalValidationScreen::state() avant cette etape, voir
  // HANDOFF_PROGRESS.md).
  const AppState& state() const { return state_; }

 private:
  AppState state_;
  static constexpr uint32_t kSyncDwellMs = 1500;

  trophy::ProfileData mapProfile(const AppState& state) const;
  std::string formatUpdated(AppLanguage lang) const;

  uint32_t nowMillis_ = 0;

  // Retour automatique Dashboard apres un succes affiche sur l'ecran Sync.
  bool syncDwellPending_ = false;
  uint32_t syncDwellDueMs_ = 0;

  // Detecte la transition VERS kOffline (front montant), pas seulement
  // l'etat courant : sans ca, tant que le reseau reste hors ligne (cas reel
  // et attendu sans Wi-Fi configure), chaque tick rappelait
  // showSyncState(kOffline) et renvoyait immediatement sur l'ecran Hors
  // ligne, meme juste apres que l'utilisateur ait tape "Retour dashboard" --
  // le bouton semblait ne rien faire. Bug reel trouve le 2026-07-23 lors du
  // premier essai materiel (Wi-Fi jamais configure sur une carte neuve).
  bool wasOffline_ = false;

  // Suivi du dernier succes reel pour l'affichage "il y a Xmin" (relatif,
  // pas d'epoch necessaire -- voir formatUpdated()).
  bool everSynced_ = false;
  uint32_t lastSyncMillis_ = 0;
  uint32_t lastSeenSyncEpoch_ = 0;

  // Evite de re-annoncer/re-empiler la meme erreur a chaque tick tant
  // qu'elle n'a pas change (AppController::publishState() rappelle
  // showError() tant que lastError.hasError() reste vrai -- voir
  // src/app/AppController.cpp) : App::set_error() du design force sinon un
  // rebuild d'ecran a chaque appel, meme sans changement reel.
  int lastErrorCode_ = 0;
  std::string lastErrorMessage_;
};
