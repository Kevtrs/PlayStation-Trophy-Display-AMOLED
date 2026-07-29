#include "storage/NvsPersistentStore.h"

#include <FS.h>
#include <LittleFS.h>

#include "utils/Logger.h"

NvsPersistentStore::NvsPersistentStore() {
  // true = formate automatiquement si le montage echoue (premiere utilisation
  // ou partition corrompue) -- voir docs/CACHE.md et docs/CONFIGURATION.md.
  //
  // Le 4e parametre (nom de partition) DOIT correspondre au nom déclaré
  // dans partitions.csv ("littlefs") : LittleFS.begin() cherche par defaut
  // une partition nommee "spiffs" (voir sa signature dans
  // framework-arduinoespressif32/libraries/LittleFS/src/LittleFS.h), qui
  // n'existe pas dans notre table -- le montage echouait donc a chaque
  // demarrage (aucune partition trouvee, pas juste un probleme de format),
  // et le formatage automatique qui suit un montage rate effacait aussitot
  // tout contenu deja ecrit (data/index.html, configuration sauvegardee...).
  // Bug reel trouve le 2026-07-24 : /api/diagnostics rapportait
  // littleFsTotalBytes=0 malgre un uploadfs reussi et verifie.
  mounted_ = LittleFS.begin(true, "/littlefs", 10, "littlefs");
  if (!mounted_) {
    Logger::error("NvsPersistentStore: echec de montage LittleFS (meme apres formatage)");
  }
}

std::string NvsPersistentStore::pathFor(const std::string& key) const { return "/" + key + ".json"; }

bool NvsPersistentStore::load(const std::string& key, std::string& outContent) {
  if (!mounted_) return false;
  File f = LittleFS.open(pathFor(key).c_str(), "r");
  if (!f || f.isDirectory()) return false;
  outContent = f.readString().c_str();
  f.close();
  return true;
}

bool NvsPersistentStore::save(const std::string& key, const std::string& content) {
  if (!mounted_) return false;
  std::string finalPath = pathFor(key);
  std::string tmpPath = finalPath + ".tmp";

  File f = LittleFS.open(tmpPath.c_str(), "w");
  if (!f) return false;
  size_t written = f.print(content.c_str());
  f.close();
  if (written != content.size()) {
    LittleFS.remove(tmpPath.c_str());
    return false;
  }

  // "Renommage" atomique : LittleFS.rename() remplace la destination si
  // elle existe deja.
  LittleFS.remove(finalPath.c_str());
  return LittleFS.rename(tmpPath.c_str(), finalPath.c_str());
}

bool NvsPersistentStore::remove(const std::string& key) {
  if (!mounted_) return false;
  return LittleFS.remove(pathFor(key).c_str());
}
