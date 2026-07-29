#include "storage/FilePersistentStore.h"

#include <filesystem>
#include <fstream>
#include <sstream>

FilePersistentStore::FilePersistentStore(std::string baseDir) : baseDir_(std::move(baseDir)) {
  std::error_code ec;
  std::filesystem::create_directories(baseDir_, ec);
}

std::string FilePersistentStore::pathFor(const std::string& key) const { return baseDir_ + "/" + key + ".json"; }

bool FilePersistentStore::load(const std::string& key, std::string& outContent) {
  std::ifstream in(pathFor(key), std::ios::binary);
  if (!in.is_open()) return false;
  std::ostringstream ss;
  ss << in.rdbuf();
  outContent = ss.str();
  return true;
}

bool FilePersistentStore::save(const std::string& key, const std::string& content) {
  // Ecriture atomique : fichier temporaire puis renommage (evite un fichier
  // corrompu/tronque si le processus est interrompu en plein milieu).
  std::string finalPath = pathFor(key);
  std::string tmpPath = finalPath + ".tmp";
  {
    std::ofstream out(tmpPath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;
    out << content;
    if (!out.good()) return false;
  }
  std::error_code ec;
  std::filesystem::rename(tmpPath, finalPath, ec);
  return !ec;
}

bool FilePersistentStore::remove(const std::string& key) {
  std::error_code ec;
  return std::filesystem::remove(pathFor(key), ec);
}
