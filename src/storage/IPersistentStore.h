#pragma once

#include <string>

// Abstraction minimale de stockage cle/valeur (une valeur = un blob texte,
// typiquement du JSON). Permet a ConfigManager et TrophyCache de rester
// portables (aucune dependance Arduino/LittleFS/NVS directe) -- voir
// docs/IMPLEMENTATION_PLAN.md.
//
// Implementations :
//   - FilePersistentStore  (portable, simulateur + tests, un fichier par cle)
//   - NvsPersistentStore   (firmware ESP32, LittleFS -- voir src/storage/NvsPersistentStore.h)
class IPersistentStore {
 public:
  virtual ~IPersistentStore() = default;

  virtual bool load(const std::string& key, std::string& outContent) = 0;
  virtual bool save(const std::string& key, const std::string& content) = 0;
  virtual bool remove(const std::string& key) = 0;
};
