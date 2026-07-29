#include "data/PocketPsnProvider.h"

#include "data/PocketPsnParser.h"
#include "utils/Logger.h"

namespace {
// Endpoint et parametres confirmes par inspection passive du firmware
// officiel (voir docs/POCKETPSN_PROTOCOL.md). Ne pas ajouter d'autre route
// ou parametre sans nouvelle preuve.
constexpr const char* kEndpoint = "https://api.pocketpsn.com/PSTrophyDisplay/";
constexpr int kHttpTimeoutMs = 8000;
constexpr size_t kMaxResponseBytes = 64 * 1024;  // garde-fou, voir brief section 15
}  // namespace

PocketPsnProvider::PocketPsnProvider(std::string psnUsername, std::string apiKey, IPocketPsnHttpClient& httpClient)
    : psnUsername_(std::move(psnUsername)), apiKey_(std::move(apiKey)), httpClient_(httpClient) {}

void PocketPsnProvider::requestRefresh() {
  status_ = Status::kInProgress;
  lastErrorCode_ = kErrorNone;
  lastError_.clear();

  std::string body = "psn_name=" + psnUsername_ + "&key=" + apiKey_;
  IPocketPsnHttpClient::Response response =
      httpClient_.post(kEndpoint, body, "application/x-www-form-urlencoded", kHttpTimeoutMs);

  if (!response.transportOk) {
    status_ = Status::kError;
    lastErrorCode_ = kErrorHttpBeginFailed;
    lastError_ = "Erreur reseau/transport";
    return;
  }

  Logger::info("PocketPsnProvider: HTTP %d, Content-Type='%s', %u octet(s) recus", response.httpStatus,
               response.contentType.c_str(), response.body.size());

  lastErrorCode_ = response.httpStatus;
  if (response.httpStatus != 200) {
    status_ = Status::kError;
    lastError_ = "Code HTTP inattendu: " + std::to_string(response.httpStatus);
    return;
  }

  if (response.body.size() > kMaxResponseBytes) {
    status_ = Status::kError;
    lastErrorCode_ = kErrorInvalidJson;
    lastError_ = "Reponse anormalement grande, rejetee par securite";
    return;
  }

  if (response.body.empty()) {
    // Comportement reellement observe (voir docs/POCKETPSN_PROTOCOL.md) pour
    // une cle/pseudo invalide : HTTP 200 + corps vide. Impossible de
    // distinguer "cle invalide" de "pseudo inconnu" a ce stade.
    status_ = Status::kError;
    lastErrorCode_ = kErrorEmptyResponse;
    lastError_ = "Reponse vide (cle ou pseudo probablement invalide)";
    return;
  }

  if (response.contentType.find("json") == std::string::npos && !response.contentType.empty()) {
    Logger::warn("PocketPsnProvider: Content-Type inattendu ('%s'), tentative de parsing quand meme",
                 response.contentType.c_str());
  }

  ProfileData parsedProfile;
  TrophyStats parsedStats;
  if (!parseResponse(response.body, parsedProfile, parsedStats)) {
    status_ = Status::kError;
    lastErrorCode_ = kErrorInvalidJson;
    lastError_ = "JSON invalide ou champs attendus absents";
    return;
  }

  profile_ = parsedProfile;
  stats_ = parsedStats;
  status_ = Status::kSuccess;

  // Confirmation reelle et definitive du schema : ne redevient jamais
  // false ensuite (une panne reseau ulterieure ponctuelle ne remet pas en
  // cause un schema deja prouve correct par un vrai succes). Voir
  // PocketPsnProvider.h pour la semantique complete.
  if (!profile_.username.empty()) {
    verified_ = true;
  }
}

TrophyDataProvider::Status PocketPsnProvider::poll() { return status_; }

bool PocketPsnProvider::parseResponse(const std::string& json, ProfileData& outProfile, TrophyStats& outStats) {
  PocketPsnParser::ParseResult result = PocketPsnParser::parse(json);
  if (!result.ok()) {
    lastError_ = result.errorMessage;
    return false;
  }
  outProfile = result.profile;
  outStats = result.stats;
  return true;
}
