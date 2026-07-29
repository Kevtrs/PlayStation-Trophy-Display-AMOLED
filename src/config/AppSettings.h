#pragma once

#include <string>

// Reglages persistants (voir ConfigManager). Valeurs par defaut sures :
// mode demo actif, pas de reseau configure -- l'appareil doit pouvoir
// demarrer et s'afficher sans aucune configuration prealable.
enum class AppLanguage { kFrench, kEnglish };

struct AppSettings {
  static constexpr int kCurrentSchemaVersion = 1;

  int schemaVersion = kCurrentSchemaVersion;

  // Reseau -- jamais renvoyes par une API (voir ConfigManager::toPublicJson).
  std::string wifiSsid;
  std::string wifiPassword;

  // Pocket PSN -- voir AUDIT.md section 3 pour l'obtention de la cle.
  std::string psnUsername;
  std::string pocketPsnApiKey;

  // Affichage / comportement.
  AppLanguage language = AppLanguage::kFrench;
  int brightnessPercent = 70;
  int sleepTimeoutSeconds = 180;
  bool animationsEnabled = true;
  bool autoRotateEnabled = true;
  int rotationIntervalSeconds = 10;

  // Synchronisation.
  int syncIntervalMinutes = 30;
  std::string timezone = "Europe/Paris";

  // Mode demo : actif par defaut tant que Pocket PSN n'est pas configure.
  bool demoMode = true;
};
