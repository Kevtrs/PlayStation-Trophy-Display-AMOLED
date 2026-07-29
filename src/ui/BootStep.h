#pragma once

// Etapes reelles du demarrage, dans l'ordre ou AppController::begin()/tick()
// les traverse (voir AppController.cpp) -- jamais de progression simulee par
// le temps. Purement structurel (aucune dependance LVGL/texte UI) : c'est a
// l'implementation de UiBridge de choisir le pourcentage et le texte affiches
// (voir RoundUiBridge::showBootProgress), exactement comme AppError laisse
// la traduction en texte utilisateur a l'UI plutot qu'a AppController.
enum class BootStep {
  kSystemStart,    // demarrage systeme (avant tout chargement)
  kConfigLoaded,   // configuration chargee (ConfigManager::load())
  kCacheLoaded,    // cache hors-ligne charge (TrophyRepository::loadFromCache())
  kNetworkInit,    // initialisation reseau demarree (IWiFiManager::begin())
  kProfileLoaded,  // profil/stats initiaux disponibles (cache ou vides)
  kDataReady,      // recuperation des donnees Pocket PSN resolue (succes, hors
                   // ligne ou erreur -- jamais d'attente indefinie du reseau)
  kUiReady,        // interface prete a etre affichee
  kAppReady,       // application prete (transition vers le tableau de bord)
};
