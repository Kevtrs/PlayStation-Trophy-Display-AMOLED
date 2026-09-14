#pragma once

#include <cstdint>
#include <string>

#include "lvgl.h"
#include "screens/screens_wide.hpp"
#include "ui/AppState.h"
#include "ui/UiBridge.h"
#include "ui/ui_model.hpp"

// Implementation de UiBridge pour le board 7" (800x480, voir
// screens_wide.hpp) : meme role que RoundUiBridge pour l'ecran rond, mais
// sans le framework de pages "App"/ui_model (screens_wide.cpp prend des
// donnees pures) -- fait donc defiler lui-meme, automatiquement, Dashboard
// -> 4 trophees -> 4 statistiques -> Dashboard...
//
// Deux modes, volontairement dans cet ordre de priorite :
//
//  - AMBIANT (defaut) : le defilement tourne seul, aucun controle affiche.
//    L'appareil est un objet pose sur un bureau, il doit etre lisible et
//    complet sans qu'on y touche -- c'est l'etat dans lequel il passe la
//    quasi-totalite de son temps.
//  - INTERACTIF : declenche par n'importe quel appui (le GT911 est
//    enregistre dans LVGL, voir main_7inch.cpp). Met le defilement en
//    pause, affiche la couche de navigation (fleches, points cliquables,
//    engrenage). Revient tout seul en ambiant apres kIdleResumeMs sans
//    contact, pour qu'un appareil laisse tel quel ne reste jamais bloque
//    sur un ecran.
//
// Le tactile n'est donc jamais indispensable : il ouvre des raccourcis
// (aller directement a un ecran, atteindre les Reglages) sans qu'aucune
// information ne devienne inaccessible a qui ne touche rien.
//
// Aucune page Boot/Sync/Erreur dediee pour l'instant (voir Key Technical
// Concepts, seuls Dashboard/Trophees/Statistiques sont dessines cote large)
// : ces transitions sont journalisees plutot qu'affichees, le dernier
// ecran de donnees connu reste visible -- un choix honnete plutot qu'un
// ecran invente non valide par l'utilisateur.
class WideUiBridge : public UiBridge {
 public:
  // A appeler une fois, apres AppController::begin() : affiche le premier
  // slide (Dashboard). Doit etre appele avec le verrou esp_lv_adapter deja
  // pris (voir main_7inch.cpp) puisque cette methode touche LVGL.
  void begin();

  // A appeler a chaque tick de la boucle principale, apres
  // AppController::tick() : fait avancer le defilement automatique. Doit
  // aussi etre appele avec le verrou esp_lv_adapter pris.
  void tick(uint32_t nowMillis);

  // UiBridge
  void setAppState(const AppState& state) override;
  void showSyncState(SyncState state) override;
  void showTrophyDelta(const TrophyDelta& delta) override;
  void showError(const AppError& error) override;
  void showBootProgress(BootStep step) override;

 private:
  trophy::ProfileData mapProfile(const AppState& state) const;
  std::string formatUpdated(AppLanguage lang) const;
  void loadSlide(int index);
  void loadWifiSetupScreen();

  // --- Tactile ---------------------------------------------------------
  // Point d'entree unique des controles de la couche de navigation
  // (trampoline C -> membre, LVGL ne connait que des fonctions libres).
  static void onNavAction(trophy::WideNavAction action, int target, void* owner);
  static void onScreenGesture(lv_event_t* e);
  void handleNav(trophy::WideNavAction action, int target);
  // Applique l'action en attente, depuis tick() uniquement.
  void applyPendingNav();
  // Enregistre un contact et bascule en mode interactif si besoin.
  void noteTouchActivity();
  // Lit l'etat du pointeur LVGL a chaque tick : un appui sur une carte est
  // absorbe par cette carte et ne remonte pas jusqu'a l'ecran, donc un
  // callback pose sur la racine raterait la moitie des appuis.
  void pollTouchActivity();
  void setInteractive(bool interactive);
  void attachNavLayer(lv_obj_t* screen);
  void openSettings();
  void closeSettings();
  enum class StateScreenKind : uint8_t {
    kNone,
    kSync,
    kOffline,
    kError,
  };
  void showStateScreen(lv_obj_t* next, StateScreenKind kind);
  void clearStateScreen();
  void showScreen(lv_obj_t* next);

  trophy::ProfileData profile_;
  AppLanguage language_ = AppLanguage::kFrench;
  uint32_t nowMillis_ = 0;

  int slideIndex_ = 0;
  uint32_t lastSlideMs_ = 0;
  lv_obj_t* currentScreen_ = nullptr;
  bool began_ = false;

  // Mode interactif : defilement en pause, couche de navigation visible.
  bool interactive_ = false;
  uint32_t lastTouchMs_ = 0;
  uint32_t lastNavMs_ = 0;
  bool showingSettings_ = false;
  // Appartient a currentScreen_ : remis a nullptr des que cet ecran est
  // detruit (voir showScreen()), jamais libere separement.
  lv_obj_t* navOverlay_ = nullptr;

  // Action tactile en attente. Un callback LVGL (bouton ou geste)
  // s'execute DANS l'arbre de l'ecran courant : s'il declenchait lui-meme
  // le changement d'ecran, il detruirait l'objet en train de distribuer
  // l'evenement. Les callbacks se contentent donc d'enregistrer
  // l'intention, et tick() la realise une fois LVGL au repos.
  bool navPending_ = false;
  trophy::WideNavAction pendingAction_ = trophy::WideNavAction::kNext;
  int pendingTarget_ = 0;

  // Tant qu'aucun Wi-Fi/compte n'est configure (WifiState::kAccessPoint,
  // voir AppController::tick()), l'ecran de connexion (build_wifi_setup_
  // screen_wide) remplace ENTIEREMENT le defilement normal -- ce board n'a
  // pas de tactile, un utilisateur non-technique doit voir cette info et
  // seulement elle, pas des trophees de demo qui donneraient l'impression
  // que c'est deja configure. Retour utilisateur du 2026-07-29.
  NetworkStatus network_;
  bool showingSetup_ = false;

  // Meme detection de front (pas de niveau) que RoundUiBridge::setAppState()
  // sur lastSyncEpoch : sans ca, "updated" resterait bloque sur la valeur
  // de la toute premiere synchronisation au lieu de refleter le temps
  // ecoule reel.
  bool everSynced_ = false;
  uint32_t lastSyncMillis_ = 0;
  uint32_t lastSeenSyncEpoch_ = 0;
  bool showingStateScreen_ = false;
  StateScreenKind stateScreenKind_ = StateScreenKind::kNone;
  SyncState lastRawSyncState_ = SyncState::kIdle;
  bool syncStateDismissed_ = false;
  SyncState dismissedSyncState_ = SyncState::kIdle;
  int lastErrorCode_ = 0;
  std::string lastErrorMessage_;
};
