#include "web/WebApiHandlers.h"

#include <ArduinoJson.h>

#include <algorithm>

#include "data/ProviderFactory.h"
#include "services/TimeService.h"
#include "version.h"

namespace WebApiHandlers {

namespace {

const char* syncStateToString(SyncState state) {
  switch (state) {
    case SyncState::kIdle:
      return "idle";
    case SyncState::kWaitingForNetwork:
    case SyncState::kConnecting:
    case SyncState::kDownloading:
    case SyncState::kParsing:
    case SyncState::kSaving:
      return "syncing";
    case SyncState::kSuccess:
      return "success";
    case SyncState::kError:
      return "error";
    case SyncState::kOffline:
      return "offline";
  }
  return "unknown";
}

const char* wifiStateToString(WifiState state) {
  switch (state) {
    case WifiState::kDisconnected:
      return "disconnected";
    case WifiState::kConnecting:
      return "connecting";
    case WifiState::kConnected:
      return "connected";
    case WifiState::kAccessPoint:
      return "access_point";
    case WifiState::kError:
      return "error";
  }
  return "unknown";
}

// Ecrit une valeur optionnelle en JSON : `null` si absente (voir
// DiagnosticsSnapshot -- "non mesurable dans le simulateur"). Template sur
// le type du "proxy" ArduinoJson (celui renvoye par doc["cle"], pas
// directement JsonVariant) pour rester compatible avec ArduinoJson v7.
template <typename TVariant, typename T>
void setOptional(TVariant v, const std::optional<T>& value) {
  if (value.has_value()) {
    v.set(*value);
  } else {
    v.set(nullptr);
  }
}

std::string wifiFriendlyMessage(WifiState state) {
  switch (state) {
    case WifiState::kDisconnected:
      return "Wi-Fi deconnecte";
    case WifiState::kConnecting:
      return "Connexion en cours...";
    case WifiState::kConnected:
      return "";
    case WifiState::kAccessPoint:
      return "Point d'acces de configuration actif";
    case WifiState::kError:
      return "Erreur Wi-Fi";
  }
  return "";
}

// Champs communs a GET /api/config et a l'enveloppe de reponse de
// POST /api/config (voir data/app.js: renderConfig()).
void populateConfigObject(JsonObject config, const AppSettings& s) {
  config["ssid"] = s.wifiSsid;
  config["psnUsername"] = s.psnUsername;
  // Jamais la cle elle-meme (voir AppSettings.h / ConfigManager::toPublicJson) --
  // seulement si une cle EFFECTIVE est disponible (manuelle ou partagee
  // compilee dans le firmware, voir ProviderFactory::effectiveApiKey() et
  // AUDIT.md section 0quater), meme nom de champ que
  // ConfigManager::toPublicJson() pour rester coherent.
  config["pocketPsnKeyConfigured"] = !ProviderFactory::effectiveApiKey(s).empty();
  config["brightness"] = s.brightnessPercent;
  config["sleepEnabled"] = s.sleepTimeoutSeconds > 0;
  config["sleepDelay"] = s.sleepTimeoutSeconds > 0 ? std::max(1, s.sleepTimeoutSeconds / 60) : 5;
  config["autoRotation"] = s.autoRotateEnabled;
  config["rotationDelay"] = s.rotationIntervalSeconds;
  config["animations"] = s.animationsEnabled;
  config["language"] = s.language == AppLanguage::kFrench ? "fr" : "en";
  config["syncInterval"] = s.syncIntervalMinutes;
}

}  // namespace

std::string buildStatusJson(const AppController& appController) {
  const AppState& state = appController.state();
  bool connected = state.network.state == WifiState::kConnected;
  // "configure" au sens du module Web UI : Wi-Fi ET pseudo PSN renseignes
  // (voir index.html : "Renseignez le Wi-Fi et le pseudo PSN pour activer
  // la synchronisation").
  bool configured = !state.settings.wifiSsid.empty() && !state.settings.psnUsername.empty();
  bool syncing = isSyncActive(state.sync.state);

  JsonDocument doc;
  doc["configured"] = configured;
  doc["offline"] = state.sync.isOffline;
  if (state.sync.lastError.hasError()) {
    doc["error"] = state.sync.lastError.message;
  } else {
    doc["error"] = nullptr;
  }

  JsonObject network = doc["network"].to<JsonObject>();
  network["connected"] = connected;
  network["ssid"] = state.network.ssid;
  network["ip"] = state.network.ipAddress;
  network["message"] = wifiFriendlyMessage(state.network.state);

  JsonObject sync = doc["sync"].to<JsonObject>();
  sync["state"] = syncing ? "syncing" : syncStateToString(state.sync.state);
  if (syncing) {
    sync["lastSync"] = "syncing";
  } else if (state.sync.lastSyncEpoch == 0) {
    sync["lastSync"] = nullptr;
  } else {
    sync["lastSync"] = TimeService::formatClock(state.sync.lastSyncEpoch);
  }
  // "cache" : des donnees existent mais l'appareil est hors-ligne (voir
  // index.html : "Donnees en cache" -> "les dernieres donnees valides
  // restent visibles jusqu'au prochain acces Pocket PSN").
  sync["source"] = (state.sync.isOffline && !state.profile.username.empty()) ? "cache" : "live";

  std::string out;
  serializeJson(doc, out);
  return out;
}

std::string buildConfigJson(const AppController& appController) {
  JsonDocument doc;
  populateConfigObject(doc.to<JsonObject>(), appController.state().settings);
  std::string out;
  serializeJson(doc, out);
  return out;
}

std::string buildConfigResponseJson(const AppController& appController, const std::string& warning) {
  JsonDocument doc;
  JsonObject config = doc["config"].to<JsonObject>();
  populateConfigObject(config, appController.state().settings);
  if (!warning.empty()) {
    doc["message"] = warning;
  }
  std::string out;
  serializeJson(doc, out);
  return out;
}

bool translateConfigPatch(const std::string& webConfigBody, std::string& outInternalJson, std::string& outWarning,
                          std::string& outError) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, webConfigBody);
  if (err) {
    outError = std::string("JSON invalide: ") + err.c_str();
    return false;
  }

  JsonDocument internalDoc;

  if (doc["ssid"].is<const char*>()) internalDoc["wifiSsid"] = doc["ssid"];
  // Champ ecriture seule, meme convention que pocketPsnApiKey ci-dessous :
  // le front (data/app.js) ne l'inclut que si l'utilisateur a saisi une
  // nouvelle valeur -- absent => mot de passe deja enregistre conserve.
  // Permet d'enregistrer reseau + mot de passe + pseudo PSN en un seul
  // "Enregistrer" (voir configPatchRequiresRestart ci-dessous), sans
  // dependre du bouton "Connecter" separe qui coupe le point d'acces de
  // secours des l'appui (voir WiFiManager.cpp).
  if (doc["password"].is<const char*>()) internalDoc["wifiPassword"] = doc["password"];
  if (doc["psnUsername"].is<const char*>()) internalDoc["psnUsername"] = doc["psnUsername"];
  // Champ ecriture seule : jamais renvoye par populateConfigObject() (voir
  // "pocketPsnKeyConfigured" ci-dessus a la place). Le front n'inclut ce
  // champ dans le corps que si l'utilisateur a saisi une nouvelle valeur
  // (voir data/app.js) -- absent => cle actuelle inchangee.
  if (doc["pocketPsnApiKey"].is<const char*>()) internalDoc["pocketPsnApiKey"] = doc["pocketPsnApiKey"];
  if (doc["brightness"].is<int>()) internalDoc["brightnessPercent"] = doc["brightness"];

  // sleepEnabled/sleepDelay (module Web UI, deux champs) -> sleepTimeoutSeconds
  // (interne, un seul champ : 0 = veille desactivee).
  if (doc["sleepEnabled"].is<bool>() && doc["sleepEnabled"] == false) {
    internalDoc["sleepTimeoutSeconds"] = 0;
  } else if (doc["sleepDelay"].is<int>()) {
    internalDoc["sleepTimeoutSeconds"] = static_cast<int>(doc["sleepDelay"]) * 60;
  }

  if (doc["autoRotation"].is<bool>()) internalDoc["autoRotateEnabled"] = doc["autoRotation"];
  if (doc["rotationDelay"].is<int>()) internalDoc["rotationIntervalSeconds"] = doc["rotationDelay"];
  if (doc["animations"].is<bool>()) internalDoc["animationsEnabled"] = doc["animations"];

  if (doc["language"].is<const char*>()) {
    std::string lang = doc["language"].as<std::string>();
    if (lang == "fr" || lang == "en") {
      internalDoc["language"] = lang;
    } else {
      // Incompatibilite documentee (voir WebApiHandlers.h) : AppLanguage
      // ne supporte que fr/en. Ignoree plutot que de rejeter toute la
      // requete ou de corrompre le reglage.
      outWarning = "Langue '" + lang + "' non supportee par le firmware (seuls fr/en le sont) -- valeur conservee.";
    }
  }

  if (doc["syncInterval"].is<int>()) internalDoc["syncIntervalMinutes"] = doc["syncInterval"];

  serializeJson(internalDoc, outInternalJson);
  return true;
}

bool configPatchRequiresRestart(const std::string& internalJson) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, internalJson);
  if (err) return false;
  // Champs qui influencent ProviderFactory::shouldUsePocketPsn() : leur
  // effet n'est applique qu'au demarrage (voir AUDIT.md section 0ter), un
  // redemarrage est donc necessaire pour qu'un changement prenne effet.
  return doc["psnUsername"].is<const char*>() || doc["pocketPsnApiKey"].is<const char*>();
}

std::string buildWifiScanJson(const IWiFiManager& wifi) {
  JsonDocument doc;
  switch (wifi.scanState()) {
    case ScanState::kIdle:
      doc["status"] = "idle";
      break;
    case ScanState::kScanning:
      doc["status"] = "scanning";
      break;
    case ScanState::kDone:
      doc["status"] = "done";
      break;
  }

  JsonArray networks = doc["networks"].to<JsonArray>();
  if (wifi.scanState() == ScanState::kDone) {
    for (const NetworkScanResult& network : wifi.scanResults()) {
      JsonObject entry = networks.add<JsonObject>();
      entry["ssid"] = network.ssid;
      entry["rssi"] = network.rssiDbm;
      entry["secure"] = network.requiresPassword;
    }
  }

  std::string out;
  serializeJson(doc, out);
  return out;
}

bool parseWifiConnectRequest(const std::string& body, std::string& outSsid, std::string& outPassword,
                             std::string& outError) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    outError = std::string("JSON invalide: ") + err.c_str();
    return false;
  }

  if (!doc["ssid"].is<const char*>() || doc["ssid"].as<std::string>().empty()) {
    outError = "Champ 'ssid' absent ou vide";
    return false;
  }

  outSsid = doc["ssid"].as<std::string>();
  outPassword = doc["password"] | "";
  return true;
}

std::string buildSimpleResultJson(bool ok, const std::string& message) {
  JsonDocument doc;
  doc["ok"] = ok;
  if (!message.empty()) {
    doc["message"] = message;
  }
  std::string out;
  serializeJson(doc, out);
  return out;
}

std::string buildNotImplementedJson(const std::string& message) {
  JsonDocument doc;
  doc["status"] = "not_implemented";
  doc["message"] = message;
  std::string out;
  serializeJson(doc, out);
  return out;
}

bool shouldAcceptSyncRequest(SyncState currentState) { return !isSyncActive(currentState); }

std::string buildSyncResponseJson(bool accepted, const std::string& message) {
  JsonDocument doc;
  doc["ok"] = accepted;
  doc["message"] = message;
  std::string out;
  serializeJson(doc, out);
  return out;
}

bool parseResetConfirmation(const std::string& body, bool& outConfirmed, std::string& outError) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    outError = std::string("JSON invalide: ") + err.c_str();
    outConfirmed = false;
    return false;
  }
  outConfirmed = doc["confirm"] | false;
  return true;
}

std::string buildDiagnosticsJson(const DiagnosticsSnapshot& s) {
  JsonDocument doc;
  doc["firmwareVersion"] = s.firmwareVersion;

  setOptional(doc["uptimeSeconds"], s.uptimeSeconds);
  setOptional(doc["freeHeapBytes"], s.freeHeapBytes);
  setOptional(doc["minFreeHeapBytes"], s.minFreeHeapBytes);
  setOptional(doc["psramTotalBytes"], s.psramTotalBytes);
  setOptional(doc["psramFreeBytes"], s.psramFreeBytes);
  setOptional(doc["flashSizeBytes"], s.flashSizeBytes);
  setOptional(doc["appUsedBytes"], s.appUsedBytes);
  setOptional(doc["littleFsTotalBytes"], s.littleFsTotalBytes);
  setOptional(doc["littleFsUsedBytes"], s.littleFsUsedBytes);

  doc["wifiState"] = s.wifiState;
  doc["ssid"] = s.ssid;
  doc["ipAddress"] = s.ipAddress;
  doc["rssi"] = s.rssi;

  doc["cacheAvailable"] = s.cacheAvailable;
  setOptional(doc["cacheAgeSeconds"], s.cacheAgeSeconds);

  doc["lastSyncState"] = s.lastSyncState;
  doc["lastSyncTimestamp"] = s.lastSyncTimestamp;
  setOptional(doc["lastHttpStatus"], s.lastHttpStatus);
  doc["lastErrorCode"] = s.lastErrorCode;
  doc["syncSuccessCount"] = s.syncSuccessCount;
  doc["syncFailureCount"] = s.syncFailureCount;

  std::string out;
  serializeJson(doc, out);
  return out;
}

DiagnosticsSnapshot buildDiagnosticsSnapshot(const AppController& appController, uint32_t nowEpoch) {
  const AppState& state = appController.state();

  DiagnosticsSnapshot snapshot;
  snapshot.firmwareVersion = kFirmwareVersion;

  // Champs materiel (heap/PSRAM/Flash/LittleFS/uptime) : volontairement
  // laisses a std::nullopt ici -- non mesurables depuis cette couche
  // portable (voir DiagnosticsSnapshot.h). Le simulateur les laisse tels
  // quels ("non mesurable dans le simulateur", comme demande) ;
  // CaptivePortalServer (firmware) les complete avant serialisation.

  snapshot.wifiState = wifiStateToString(state.network.state);
  snapshot.ssid = state.network.ssid;
  snapshot.ipAddress = state.network.ipAddress;
  snapshot.rssi = state.network.rssiDbm;

  snapshot.cacheAvailable = appController.hasCachedData();
  int32_t age = -1;
  uint32_t fetchEpoch = appController.lastCacheFetchEpoch();
  if (snapshot.cacheAvailable && nowEpoch > 0 && fetchEpoch > 0) {
    age = nowEpoch >= fetchEpoch ? static_cast<int32_t>(nowEpoch - fetchEpoch) : 0;
  }
  if (age >= 0) {
    snapshot.cacheAgeSeconds = age;
  }

  snapshot.lastSyncState = syncStateToString(state.sync.state);
  snapshot.lastSyncTimestamp = state.sync.lastSyncEpoch;
  // lastHttpStatus reste std::nullopt (voir DiagnosticsSnapshot.h) : pas
  // encore distinct de lastErrorCode dans l'architecture actuelle.
  snapshot.lastErrorCode = state.sync.lastError.code;
  snapshot.syncSuccessCount = appController.syncTotalCount() - appController.syncFailureCount();
  snapshot.syncFailureCount = appController.syncFailureCount();

  return snapshot;
}

}  // namespace WebApiHandlers
