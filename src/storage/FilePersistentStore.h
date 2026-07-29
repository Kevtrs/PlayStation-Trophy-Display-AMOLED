#pragma once

#include <string>

#include "storage/IPersistentStore.h"

// Backend portable (std::ifstream/ofstream) : simulateur PC et tests. Un
// fichier "<baseDir>/<key>.json" par cle. Jamais compile pour le firmware
// (exclu via build_src_filter dans platformio.ini).
class FilePersistentStore : public IPersistentStore {
 public:
  explicit FilePersistentStore(std::string baseDir);

  bool load(const std::string& key, std::string& outContent) override;
  bool save(const std::string& key, const std::string& content) override;
  bool remove(const std::string& key) override;

 private:
  std::string pathFor(const std::string& key) const;
  std::string baseDir_;
};
