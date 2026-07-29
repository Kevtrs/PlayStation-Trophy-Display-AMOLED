#pragma once

#include "config/AppSettings.h"
#include "storage/IPersistentStore.h"

// Gestionnaire de configuration versionne (voir docs/CONFIGURATION.md).
// Portable : ne depend que d'IPersistentStore (implementation fournie par
// la plateforme -- NvsPersistentStore sur firmware, FilePersistentStore sur
// simulateur/tests) et d'ArduinoJson (portable).
class ConfigManager {
 public:
  explicit ConfigManager(IPersistentStore& store);

  // Charge la configuration depuis le stockage. En cas d'absence ou de
  // corruption, applique des valeurs par defaut sures et renvoie false (la
  // config par defaut reste utilisable -- ne bloque jamais le demarrage).
  bool load();

  // Sauvegarde atomique (voir FilePersistentStore/NvsPersistentStore).
  bool save();

  // Reinitialisation complete aux valeurs par defaut (n'ecrit sur le
  // stockage que si persistNow=true).
  void resetToDefaults(bool persistNow);

  const AppSettings& settings() const { return settings_; }
  AppSettings& mutableSettings() { return settings_; }

  // Version JSON sans les secrets (mot de passe Wi-Fi, cle Pocket PSN) --
  // utilisee par l'API web GET /api/config (voir docs/WEB_UI.md). Ne
  // jamais utiliser toPublicJson() pour la persistance : utiliser save().
  std::string toPublicJson() const;

  // Valide et applique un JSON recu de l'API web (POST /api/config) :
  // seuls les champs presents et valides sont mis a jour, les autres
  // restent inchanges. Renvoie false si le JSON est invalide.
  bool applyJsonPatch(const std::string& json, std::string& outError);

  static constexpr const char* kStoreKey = "config";

 private:
  bool validate(AppSettings& s) const;
  bool migrate(AppSettings& s, int fromVersion) const;

  IPersistentStore& store_;
  AppSettings settings_;
};
