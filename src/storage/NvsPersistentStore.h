#pragma once

#include "storage/IPersistentStore.h"

// Backend firmware (LittleFS, partition dediee -- voir partitions.csv). Un
// fichier "/<key>.json" par cle. Jamais compile pour le simulateur (absent
// du glob de simulator/CMakeLists.txt).
class NvsPersistentStore : public IPersistentStore {
 public:
  NvsPersistentStore();

  bool load(const std::string& key, std::string& outContent) override;
  bool save(const std::string& key, const std::string& content) override;
  bool remove(const std::string& key) override;

 private:
  bool mounted_ = false;
  std::string pathFor(const std::string& key) const;
};
