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
// -> 4 trophees -> 4 statistiques -> Dashboard... (aucun tactile requis,
// voir la decision de design du 2026-07-28).
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
  void showScreen(lv_obj_t* next);

  trophy::ProfileData profile_;
  AppLanguage language_ = AppLanguage::kFrench;
  uint32_t nowMillis_ = 0;

  int slideIndex_ = 0;
  uint32_t lastSlideMs_ = 0;
  lv_obj_t* currentScreen_ = nullptr;
  bool began_ = false;

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
};
