#include "storage/TrophyCache.h"

#include <ArduinoJson.h>

#include "utils/Logger.h"

namespace {
// FNV-1a 32 bits : detection simple de corruption (au-dela de la seule
// validite JSON -- un octet altere dans un champ numerique produirait un
// JSON toujours valide mais un contenu incorrect).
uint32_t fnv1a(const std::string& data) {
  uint32_t hash = 2166136261u;
  for (unsigned char c : data) {
    hash ^= c;
    hash *= 16777619u;
  }
  return hash;
}

std::string serializePayload(const ProfileData& profile, const TrophyStats& stats) {
  JsonDocument doc;
  doc["username"] = profile.username;
  doc["country"] = profile.country;
  doc["level"] = profile.level;
  doc["levelProgressPercent"] = profile.levelProgressPercent;
  doc["levelRemainingPoints"] = profile.levelRemainingPoints;
  doc["hasPsPlus"] = profile.hasPsPlus;
  doc["displayName"] = profile.displayName;
  doc["avatarFileName"] = profile.avatarFileName;

  doc["platinum"] = stats.platinum;
  doc["gold"] = stats.gold;
  doc["silver"] = stats.silver;
  doc["bronze"] = stats.bronze;
  doc["totalTrophies"] = stats.totalTrophies;
  doc["trophyPoints"] = stats.trophyPoints;
  doc["pocketPoints"] = stats.pocketPoints;
  doc["totalGames"] = stats.totalGames;
  doc["worldRank"] = stats.worldRank;
  doc["nationalRank"] = stats.nationalRank;
  doc["gamesCompleted"] = stats.gamesCompleted;
  doc["completionRatePercent"] = stats.completionRatePercent;
  doc["averageRarityPercent"] = stats.averageRarityPercent;
  doc["unearnedTrophies"] = stats.unearnedTrophies;
  doc["playtimeHours"] = stats.playtimeHours;
  doc["trophiesPerDay"] = stats.trophiesPerDay;

  std::string out;
  serializeJson(doc, out);
  return out;
}

bool deserializePayload(const std::string& payload, ProfileData& profile, TrophyStats& stats) {
  JsonDocument doc;
  if (deserializeJson(doc, payload)) return false;
  if (!doc["username"].is<const char*>()) return false;

  profile.username = doc["username"] | "";
  profile.country = doc["country"] | "";
  profile.level = doc["level"] | 0;
  profile.levelProgressPercent = doc["levelProgressPercent"] | 0;
  profile.levelRemainingPoints = doc["levelRemainingPoints"] | 0;
  profile.hasPsPlus = doc["hasPsPlus"] | false;
  profile.displayName = doc["displayName"] | "";
  profile.avatarFileName = doc["avatarFileName"] | "";

  stats.platinum = doc["platinum"] | 0;
  stats.gold = doc["gold"] | 0;
  stats.silver = doc["silver"] | 0;
  stats.bronze = doc["bronze"] | 0;
  stats.totalTrophies = doc["totalTrophies"] | 0;
  stats.trophyPoints = doc["trophyPoints"] | 0;
  stats.pocketPoints = doc["pocketPoints"] | 0;
  stats.totalGames = doc["totalGames"] | 0;
  stats.worldRank = doc["worldRank"] | 0;
  stats.nationalRank = doc["nationalRank"] | 0;
  stats.gamesCompleted = doc["gamesCompleted"] | 0;
  stats.completionRatePercent = doc["completionRatePercent"] | 0.0f;
  stats.averageRarityPercent = doc["averageRarityPercent"] | 0.0f;
  stats.unearnedTrophies = doc["unearnedTrophies"] | 0;
  stats.playtimeHours = doc["playtimeHours"] | 0.0f;
  stats.trophiesPerDay = doc["trophiesPerDay"] | 0.0f;
  return true;
}
}  // namespace

TrophyCache::TrophyCache(IPersistentStore& store) : store_(store) {}

bool TrophyCache::load() {
  std::string raw;
  if (!store_.load(kStoreKey, raw)) {
    Logger::info("TrophyCache: aucun cache existant (premier demarrage)");
    return false;
  }

  JsonDocument envelope;
  if (deserializeJson(envelope, raw)) {
    Logger::error("TrophyCache: enveloppe corrompue, cache ignore");
    return false;
  }

  int schemaVersion = envelope["schemaVersion"] | 0;
  uint32_t storedChecksum = envelope["checksum"] | 0u;
  uint32_t epoch = envelope["fetchEpoch"] | 0u;
  std::string payload = envelope["payload"] | "";

  if (schemaVersion != kCurrentSchemaVersion || payload.empty()) {
    Logger::error("TrophyCache: schema/payload invalide, cache ignore");
    return false;
  }

  if (fnv1a(payload) != storedChecksum) {
    Logger::error("TrophyCache: checksum invalide, cache corrompu ignore");
    return false;
  }

  ProfileData profile;
  TrophyStats stats;
  if (!deserializePayload(payload, profile, stats)) {
    Logger::error("TrophyCache: contenu illisible, cache ignore");
    return false;
  }

  profile_ = profile;
  stats_ = stats;
  fetchEpoch_ = epoch;
  hasData_ = true;
  Logger::info("TrophyCache: cache charge (fetchEpoch=%u)", static_cast<unsigned>(epoch));
  return true;
}

bool TrophyCache::save(const ProfileData& profile, const TrophyStats& stats, uint32_t fetchEpoch) {
  std::string payload = serializePayload(profile, stats);
  uint32_t checksum = fnv1a(payload);

  JsonDocument envelope;
  envelope["schemaVersion"] = kCurrentSchemaVersion;
  envelope["fetchEpoch"] = fetchEpoch;
  envelope["checksum"] = checksum;
  envelope["payload"] = payload;

  std::string out;
  serializeJson(envelope, out);

  if (!store_.save(kStoreKey, out)) {
    Logger::error("TrophyCache: echec de sauvegarde, cache precedent conserve en memoire");
    return false;
  }

  // Le cache en memoire n'est remplace qu'apres l'ecriture reussie : les
  // donnees precedentes (valides) ne sont jamais perdues en cas d'echec.
  profile_ = profile;
  stats_ = stats;
  fetchEpoch_ = fetchEpoch;
  hasData_ = true;
  return true;
}

void TrophyCache::clear() {
  store_.remove(kStoreKey);
  hasData_ = false;
  profile_ = ProfileData{};
  stats_ = TrophyStats{};
  fetchEpoch_ = 0;
  Logger::info("TrophyCache: cache efface (reinitialisation)");
}

int32_t TrophyCache::dataAgeSeconds(uint32_t nowEpoch) const {
  if (fetchEpoch_ == 0 || nowEpoch == 0 || !hasData_) return -1;
  if (nowEpoch < fetchEpoch_) return 0;
  return static_cast<int32_t>(nowEpoch - fetchEpoch_);
}
