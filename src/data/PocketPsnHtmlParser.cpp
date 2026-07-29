#include "data/PocketPsnHtmlParser.h"

#include <regex>

namespace PocketPsnHtmlParser {

namespace {

std::string trim(const std::string& s) {
  size_t start = s.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) return "";
  size_t end = s.find_last_not_of(" \t\r\n");
  return s.substr(start, end - start + 1);
}

// Retire les separateurs de milliers (virgule, espace, espace insecable
// UTF-8 C2 A0) et le signe pourcent, puis convertit en nombre. Accepte
// "3,381", "2 655", "21.95%", "2,568.28", "48.3".
bool parseLocaleNumber(const std::string& raw, double* outValue) {
  std::string cleaned;
  cleaned.reserve(raw.size());
  for (size_t i = 0; i < raw.size(); ++i) {
    unsigned char c = static_cast<unsigned char>(raw[i]);
    if (c == ',' || c == ' ' || c == '%') continue;
    if (c == 0xC2 && i + 1 < raw.size() && static_cast<unsigned char>(raw[i + 1]) == 0xA0) {
      ++i;  // espace insecable UTF-8 (0xC2 0xA0)
      continue;
    }
    cleaned += static_cast<char>(c);
  }
  cleaned = trim(cleaned);
  if (cleaned.empty()) return false;
  try {
    size_t consumed = 0;
    double value = std::stod(cleaned, &consumed);
    if (consumed != cleaned.size()) return false;
    *outValue = value;
    return true;
  } catch (...) {
    return false;
  }
}

// "<h4 ...>VALEUR</h4> ... <p ...><small>LABEL</small>" : la valeur
// precede toujours le libelle dans ce gabarit, jamais l'inverse. Ancre sur
// le libelle plutot que sur la position ordinale du <h4> pour rester
// robuste si d'autres cartes sont ajoutees a la page.
bool findLabelValue(const std::string& html, const std::string& label, std::string* outRaw) {
  // Le contenu du <h4> est toujours un texte nu (jamais de balise imbriquee)
  // : capturer avec [^<]* plutot que [\s\S]*? evite qu'un <h4> non lie plus
  // haut dans la page (ex: le pseudo) ne s'etende par erreur jusqu'a ce
  // libelle en avalant tout le HTML intermediaire.
  std::regex re("<h4[^>]*>([^<]*)</h4>\\s*<p[^>]*>\\s*<small[^>]*>\\s*" + label + "\\s*</small>");
  std::smatch match;
  if (!std::regex_search(html, match, re)) return false;
  *outRaw = trim(match[1].str());
  return true;
}

bool hasProfileMarkers(const std::string& html) {
  return html.find("user-profile-info") != std::string::npos &&
         html.find("ri-trophy-fill") != std::string::npos;
}

const char* kCloudflareTitleMarker = "Just a moment";
const char* kCloudflareScriptMarker = "/cdn-cgi/challenge-platform/";
// Fragment sans accents de "Verification de securite" (variante localisee
// possible du defi Cloudflare) -- jamais observe reellement, ajoute par
// prudence sur demande explicite.
const char* kCloudflareFrenchMarker = "rification de s";

}  // namespace

bool isCloudflareChallenge(const std::string& html, int httpStatusCode) {
  if (httpStatusCode == 403 || httpStatusCode == 429 || httpStatusCode == 503) return true;
  if (html.find(kCloudflareTitleMarker) != std::string::npos) return true;
  if (html.find(kCloudflareScriptMarker) != std::string::npos) return true;
  if (html.find(kCloudflareFrenchMarker) != std::string::npos) return true;
  return false;
}

ParseResult parse(const std::string& html, int httpStatusCode) {
  ParseResult result;

  if (html.empty()) {
    result.error = ParseError::kEmptyInput;
    result.errorMessage = "Corps HTML vide";
    return result;
  }

  if (isCloudflareChallenge(html, httpStatusCode)) {
    result.error = ParseError::kCloudflareChallenge;
    result.errorMessage = "Defi anti-robot Cloudflare detecte, aucune tentative de contournement";
    return result;
  }

  if (!hasProfileMarkers(html)) {
    result.error = ParseError::kProfileNotFound;
    result.errorMessage = "Aucun bloc de profil identifiable dans la page (ni 'user-profile-info' ni trophees)";
    return result;
  }

  ProfileData& profile = result.profile;
  TrophyStats& stats = result.stats;
  std::smatch match;

  // --- Pseudo (bloc user-profile-info) ---
  std::regex pseudoRe("<h4 class=\"mb-0\">\\s*([^\\s<]+)");
  if (!std::regex_search(html, match, pseudoRe)) {
    result.error = ParseError::kMissingRequiredFields;
    result.errorMessage = "Champ 'username' introuvable dans le bloc user-profile-info";
    return result;
  }
  profile.username = match[1].str();

  // --- Nom d'affichage (attribut data-bs-original-title du bloc user-profile-info) ---
  std::regex displayNameRe("class=\"user-profile-info\"[^>]*data-bs-original-title=\"([^\"]+)\"");
  if (!std::regex_search(html, match, displayNameRe)) {
    result.error = ParseError::kMissingRequiredFields;
    result.errorMessage = "Champ 'displayName' introuvable (attribut data-bs-original-title)";
    return result;
  }
  profile.displayName = match[1].str();

  // --- Niveau + progression (fenetre entre user-profile-info et World Rank) ---
  size_t profileInfoPos = html.find("user-profile-info");
  size_t worldRankPos = html.find("World Rank");
  if (worldRankPos == std::string::npos || worldRankPos <= profileInfoPos) {
    result.error = ParseError::kMissingRequiredFields;
    result.errorMessage = "Impossible de delimiter la zone du profil principal (World Rank introuvable ou mal placee)";
    return result;
  }
  std::string headerWindow = html.substr(profileInfoPos, worldRankPos - profileInfoPos);

  std::regex levelRe("<strong>(\\d+)</strong>");
  if (!std::regex_search(headerWindow, match, levelRe)) {
    result.error = ParseError::kMissingRequiredFields;
    result.errorMessage = "Champ 'level' introuvable";
    return result;
  }
  profile.level = std::stoi(match[1].str());

  std::regex progressRe("aria-valuenow=\"([\\d.]+)\"");
  if (!std::regex_search(headerWindow, match, progressRe)) {
    result.error = ParseError::kMissingRequiredFields;
    result.errorMessage = "Champ 'levelProgress' introuvable";
    return result;
  }
  double levelProgress = 0.0;
  if (!parseLocaleNumber(match[1].str(), &levelProgress)) {
    result.error = ParseError::kInvalidNumber;
    result.errorMessage = "Valeur 'levelProgress' non numerique";
    return result;
  }
  if (levelProgress < 0.0 || levelProgress > 100.0) {
    result.error = ParseError::kInvalidLevelProgress;
    result.errorMessage = "levelProgress hors de l'intervalle [0, 100]";
    return result;
  }
  profile.levelProgressPercent = static_cast<int>(levelProgress);

  // --- Trophees (5 icones dans le bloc user-profile-info, identifiees par couleur, pas par position) ---
  std::regex trophyRe("<i class=\"ri-trophy-fill me-2\"([^>]*)></i><span class=\"fw-medium\">([^<]+)</span>");
  auto begin = std::sregex_iterator(headerWindow.begin(), headerWindow.end(), trophyRe);
  auto end = std::sregex_iterator();
  bool sawTotal = false, sawPlatinum = false, sawGold = false, sawSilver = false, sawBronze = false;
  for (auto it = begin; it != end; ++it) {
    std::string attrs = (*it)[1].str();
    std::string rawValue = trim((*it)[2].str());
    double value = 0.0;
    if (!parseLocaleNumber(rawValue, &value)) {
      result.error = ParseError::kInvalidNumber;
      result.errorMessage = "Valeur de trophee non numerique: '" + rawValue + "'";
      return result;
    }
    int intValue = static_cast<int>(value);
    if (attrs.find("#667FB2") != std::string::npos) {
      stats.platinum = intValue;
      sawPlatinum = true;
    } else if (attrs.find("#C2903E") != std::string::npos) {
      stats.gold = intValue;
      sawGold = true;
    } else if (attrs.find("#777777") != std::string::npos) {
      stats.silver = intValue;
      sawSilver = true;
    } else if (attrs.find("#C46438") != std::string::npos) {
      stats.bronze = intValue;
      sawBronze = true;
    } else if (!sawTotal) {
      stats.totalTrophies = intValue;
      sawTotal = true;
    }
  }
  if (!sawTotal || !sawPlatinum || !sawGold || !sawSilver || !sawBronze) {
    result.error = ParseError::kMissingRequiredFields;
    result.errorMessage = "Au moins un compteur de trophees (total/platine/or/argent/bronze) est introuvable";
    return result;
  }
  if (stats.totalTrophies != stats.platinum + stats.gold + stats.silver + stats.bronze) {
    result.error = ParseError::kTotalMismatch;
    result.errorMessage = "totalTrophies ne correspond pas a platinum + gold + silver + bronze";
    return result;
  }

  // --- Statistiques par libelle semantique (cartes du bas de page) ---
  struct LabeledIntField {
    const char* label;
    int* target;
  };
  struct LabeledFloatField {
    const char* label;
    float* target;
  };

  const LabeledIntField intFields[] = {
      {"World Rank", &stats.worldRank},
      {"Country Rank", &stats.nationalRank},
      {"Pocket Points", &stats.pocketPoints},
      {"Games Completed", &stats.gamesCompleted},
      {"Unearned Trophies", &stats.unearnedTrophies},
  };
  for (const auto& field : intFields) {
    std::string raw;
    if (!findLabelValue(html, field.label, &raw)) {
      result.error = ParseError::kMissingRequiredFields;
      result.errorMessage = std::string("Champ '") + field.label + "' introuvable";
      return result;
    }
    double value = 0.0;
    if (!parseLocaleNumber(raw, &value)) {
      result.error = ParseError::kInvalidNumber;
      result.errorMessage = std::string("Valeur '") + field.label + "' non numerique: '" + raw + "'";
      return result;
    }
    *field.target = static_cast<int>(value);
  }

  const LabeledFloatField floatFields[] = {
      {"Completion Average", &stats.completionRatePercent},
      {"Average Rarity", &stats.averageRarityPercent},
      {"Trophies Per Day", &stats.trophiesPerDay},
      {"Hours Played", &stats.playtimeHours},
  };
  for (const auto& field : floatFields) {
    std::string raw;
    if (!findLabelValue(html, field.label, &raw)) {
      result.error = ParseError::kMissingRequiredFields;
      result.errorMessage = std::string("Champ '") + field.label + "' introuvable";
      return result;
    }
    double value = 0.0;
    if (!parseLocaleNumber(raw, &value)) {
      result.error = ParseError::kInvalidNumber;
      result.errorMessage = std::string("Valeur '") + field.label + "' non numerique: '" + raw + "'";
      return result;
    }
    *field.target = static_cast<float>(value);
  }

  // --- Validation croisee : meta description ("DisplayName | Level N | M PlayStation Trophies") ---
  std::regex metaDescRe(
      "<meta name=\"description\" content=\"([^|]+)\\|\\s*Level\\s*(\\d+)\\s*\\|\\s*(\\d+)\\s*PlayStation Trophies\"");
  if (std::regex_search(html, match, metaDescRe)) {
    std::string metaDisplayName = trim(match[1].str());
    int metaLevel = std::stoi(match[2].str());
    int metaTotal = std::stoi(match[3].str());
    if (metaDisplayName != profile.displayName || metaLevel != profile.level || metaTotal != stats.totalTrophies) {
      result.error = ParseError::kMetaDescriptionMismatch;
      result.errorMessage = "La balise meta description contredit le bloc de profil principal";
      return result;
    }
  }

  result.error = ParseError::kNone;
  return result;
}

}  // namespace PocketPsnHtmlParser
