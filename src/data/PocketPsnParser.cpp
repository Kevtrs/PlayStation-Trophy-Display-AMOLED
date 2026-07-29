#include "data/PocketPsnParser.h"

#include <ArduinoJson.h>

#include <cctype>
#include <cstring>

#include "utils/Logger.h"

namespace PocketPsnParser {

namespace {

// Le backend Pocket PSN construit son JSON par concatenation cote serveur
// et laisse parfois une virgule trainante avant une accolade/crochet
// fermante -- observe reellement le 2026-07-21 sur deux requetes reelles
// independantes (voir docs/POCKETPSN_PROTOCOL.md), toujours au meme
// endroit (fin de "Quick Stats"). ArduinoJson ne garantit pas de tolerer
// ceci : on la retire nous-memes, de facon fiable, plutot que de compter
// sur une eventuelle indulgence du parseur. Parcours simple conscient des
// chaines JSON (une virgule a l'interieur d'une chaine n'est jamais
// touchee).
std::string stripTrailingCommas(const std::string& json) {
  std::string cleaned;
  cleaned.reserve(json.size());
  bool insideString = false;
  bool escaped = false;

  for (size_t i = 0; i < json.size(); ++i) {
    char c = json[i];

    if (insideString) {
      cleaned += c;
      if (escaped) {
        escaped = false;
      } else if (c == '\\') {
        escaped = true;
      } else if (c == '"') {
        insideString = false;
      }
      continue;
    }

    if (c == '"') {
      insideString = true;
      cleaned += c;
      continue;
    }

    if (c == ',') {
      size_t j = i + 1;
      while (j < json.size() && std::isspace(static_cast<unsigned char>(json[j]))) ++j;
      if (j < json.size() && (json[j] == '}' || json[j] == ']')) {
        continue;  // virgule trainante : supprimee
      }
    }

    cleaned += c;
  }

  return cleaned;
}

// Convertit une valeur "Stat" telle qu'observee reellement le 2026-07-21
// (chaine formattee : "22%", "38.76%", "10,743", "2,582") en float
// exploitable -- retire separateurs de milliers et signe pourcent.
// Accepte aussi un nombre JSON brut (defense contre une eventuelle
// evolution du format). Ne leve jamais ; logue et renvoie 0.0f si
// illisible.
float parseStatValue(JsonVariantConst value, const char* fieldNameForLog) {
  if (value.is<float>() || value.is<int>()) {
    return value.as<float>();
  }
  if (!value.is<const char*>()) {
    Logger::warn("PocketPsnParser: valeur 'Stat' de type inattendu pour '%s'", fieldNameForLog);
    return 0.0f;
  }
  std::string raw = value.as<const char*>();
  std::string cleaned;
  cleaned.reserve(raw.size());
  for (char c : raw) {
    if (c == ',' || c == '%') continue;
    cleaned += c;
  }
  if (cleaned.empty()) {
    Logger::warn("PocketPsnParser: valeur 'Stat' vide pour '%s'", fieldNameForLog);
    return 0.0f;
  }
  char* endPtr = nullptr;
  float parsed = std::strtof(cleaned.c_str(), &endPtr);
  if (endPtr == cleaned.c_str()) {
    Logger::warn("PocketPsnParser: valeur 'Stat' non numerique pour '%s': '%s'", fieldNameForLog, raw.c_str());
    return 0.0f;
  }
  return parsed;
}

// Ancien format suppose (objet unique, jamais confirme par une vraie
// reponse avant le 2026-07-21) -- conserve en repli si l'API revient un
// jour a cette forme.
void applyQuickStatsAsObject(JsonVariantConst quickStats, TrophyStats& stats) {
  stats.gamesCompleted = quickStats["Games Completed"] | 0;
  stats.completionRatePercent = quickStats["Completion Average"] | 0.0f;
  stats.averageRarityPercent = quickStats["Average Rarity"] | 0.0f;
  stats.unearnedTrophies = quickStats["Unearned Trophies"] | 0;
  stats.playtimeHours = quickStats["Hours Played"] | 0.0f;
}

// Format reellement observe le 2026-07-21 : tableau de
// {"Title": "...", "Stat": "...", "Percentile": 0.xx} -- voir
// parseStatValue() pour la conversion des valeurs "Stat".
void applyQuickStatsAsArray(JsonArrayConst quickStats, TrophyStats& stats) {
  bool sawGamesCompleted = false, sawCompletionAverage = false, sawAverageRarity = false;
  bool sawUnearnedTrophies = false, sawHoursPlayed = false;

  for (JsonVariantConst entry : quickStats) {
    const char* title = entry["Title"] | "";
    JsonVariantConst statValue = entry["Stat"];

    if (std::strcmp(title, "Games Completed") == 0) {
      stats.gamesCompleted = static_cast<int>(parseStatValue(statValue, title));
      sawGamesCompleted = true;
    } else if (std::strcmp(title, "Completion Average") == 0) {
      stats.completionRatePercent = parseStatValue(statValue, title);
      sawCompletionAverage = true;
    } else if (std::strcmp(title, "Average Rarity") == 0) {
      stats.averageRarityPercent = parseStatValue(statValue, title);
      sawAverageRarity = true;
    } else if (std::strcmp(title, "Unearned Trophies") == 0) {
      stats.unearnedTrophies = static_cast<int>(parseStatValue(statValue, title));
      sawUnearnedTrophies = true;
    } else if (std::strcmp(title, "Hours Played") == 0) {
      stats.playtimeHours = parseStatValue(statValue, title);
      sawHoursPlayed = true;
    } else if (title[0] != '\0') {
      Logger::info("PocketPsnParser: entree 'Quick Stats' non reconnue ('%s'), ignoree", title);
    }
  }

  if (!sawGamesCompleted) Logger::warn("PocketPsnParser: 'Quick Stats' sans entree 'Games Completed'");
  if (!sawCompletionAverage) Logger::warn("PocketPsnParser: 'Quick Stats' sans entree 'Completion Average'");
  if (!sawAverageRarity) Logger::warn("PocketPsnParser: 'Quick Stats' sans entree 'Average Rarity'");
  if (!sawUnearnedTrophies) Logger::warn("PocketPsnParser: 'Quick Stats' sans entree 'Unearned Trophies'");
  if (!sawHoursPlayed) Logger::warn("PocketPsnParser: 'Quick Stats' sans entree 'Hours Played'");
}

}  // namespace

ParseResult parse(const std::string& jsonBody) {
  ParseResult result;

  if (jsonBody.empty()) {
    result.error = ParseError::kEmptyInput;
    result.errorMessage = "Corps de reponse vide";
    return result;
  }

  std::string cleanedJson = stripTrailingCommas(jsonBody);

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, cleanedJson);
  if (err) {
    result.error = ParseError::kInvalidJson;
    result.errorMessage = std::string("JSON invalide: ") + err.c_str();
    return result;
  }

  // On exige au moins Username pour considerer la reponse exploitable --
  // sans cela on ne peut pas distinguer un JSON valide mais vide d'une
  // vraie reponse de profil. "Status" n'est jamais exige ici : observe
  // absent des vraies reponses de succes (present seulement, semble-t-il,
  // en cas d'erreur -- voir docs/POCKETPSN_PROTOCOL.md) ; s'il est present
  // alors que Username est absent, on l'inclut dans le message d'erreur
  // pour faciliter le diagnostic.
  if (!doc["Username"].is<const char*>()) {
    result.error = ParseError::kMissingRequiredFields;
    const char* status = doc["Status"] | "";
    if (status[0] != '\0') {
      result.errorMessage = std::string("Champ 'Username' absent de la reponse (Status='") + status + "')";
    } else {
      result.errorMessage = "Champ 'Username' absent de la reponse";
    }
    Logger::warn("PocketPsnParser: %s", result.errorMessage.c_str());
    return result;
  }

  ProfileData& profile = result.profile;
  profile.username = doc["Username"] | "";
  profile.country = doc["Country"] | "";
  profile.level = doc["PSN Level"] | 0;
  profile.levelProgressPercent = doc["PSN Level Progress"] | 0;
  profile.levelRemainingPoints = doc["PSN Level Remaining"] | 0;
  // "Plus"/"Avatar" : champs reellement observes le 2026-07-21, absents de
  // l'ancienne documentation -- jamais obligatoires, valeur par defaut si
  // absents (voir ProfileData.h).
  profile.hasPsPlus = (doc["Plus"] | 0) != 0;
  profile.avatarFileName = doc["Avatar"] | "";

  TrophyStats& stats = result.stats;
  stats.platinum = doc["Trophies Plats"] | 0;
  stats.gold = doc["Trophies Gold"] | 0;
  stats.silver = doc["Trophies Silver"] | 0;
  stats.bronze = doc["Trophies Bronze"] | 0;
  stats.totalTrophies = doc["Trophies Total"] | 0;
  stats.trophyPoints = doc["Trophy Points"] | 0;
  stats.pocketPoints = doc["Pocket Points"] | 0;
  stats.totalGames = doc["Total Games"] | 0;
  stats.worldRank = doc["World Rank"] | 0;
  stats.nationalRank = doc["Country Rank"] | 0;

  JsonVariantConst quickStats = doc["Quick Stats"];
  if (quickStats.is<JsonArrayConst>()) {
    // Format reellement observe le 2026-07-21.
    applyQuickStatsAsArray(quickStats.as<JsonArrayConst>(), stats);
  } else if (quickStats.is<JsonObjectConst>()) {
    // Ancien format suppose, jamais confirme par une vraie reponse avant
    // le 2026-07-21 -- conserve par compatibilite.
    Logger::info("PocketPsnParser: 'Quick Stats' recu sous forme d'objet (ancien format suppose)");
    applyQuickStatsAsObject(quickStats, stats);
  } else {
    Logger::warn("PocketPsnParser: champ 'Quick Stats' absent ou de type inattendu");
  }

  result.error = ParseError::kNone;
  return result;
}

}  // namespace PocketPsnParser
