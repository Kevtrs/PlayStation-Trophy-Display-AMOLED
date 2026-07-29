#include "config/ConfigManager.h"

#include <ArduinoJson.h>

#include <algorithm>

#include "utils/Logger.h"

namespace {
void toJson(const AppSettings& s, JsonDocument& doc, bool includeSecrets) {
  doc["schemaVersion"] = s.schemaVersion;
  if (includeSecrets) {
    doc["wifiSsid"] = s.wifiSsid;
    doc["wifiPassword"] = s.wifiPassword;
    doc["pocketPsnApiKey"] = s.pocketPsnApiKey;
  } else {
    doc["wifiSsid"] = s.wifiSsid;  // le SSID seul n'est pas un secret
    doc["wifiConfigured"] = !s.wifiSsid.empty();
    doc["pocketPsnKeyConfigured"] = !s.pocketPsnApiKey.empty();
  }
  doc["psnUsername"] = s.psnUsername;
  doc["language"] = s.language == AppLanguage::kFrench ? "fr" : "en";
  doc["brightnessPercent"] = s.brightnessPercent;
  doc["sleepTimeoutSeconds"] = s.sleepTimeoutSeconds;
  doc["animationsEnabled"] = s.animationsEnabled;
  doc["autoRotateEnabled"] = s.autoRotateEnabled;
  doc["rotationIntervalSeconds"] = s.rotationIntervalSeconds;
  doc["syncIntervalMinutes"] = s.syncIntervalMinutes;
  doc["timezone"] = s.timezone;
  doc["demoMode"] = s.demoMode;
}

bool fromJson(const JsonDocument& doc, AppSettings& s) {
  if (!doc["schemaVersion"].is<int>()) return false;
  s.schemaVersion = doc["schemaVersion"] | AppSettings::kCurrentSchemaVersion;
  s.wifiSsid = doc["wifiSsid"] | "";
  s.wifiPassword = doc["wifiPassword"] | "";
  s.psnUsername = doc["psnUsername"] | "";
  s.pocketPsnApiKey = doc["pocketPsnApiKey"] | "";
  std::string lang = doc["language"] | "fr";
  s.language = (lang == "en") ? AppLanguage::kEnglish : AppLanguage::kFrench;
  s.brightnessPercent = doc["brightnessPercent"] | 70;
  s.sleepTimeoutSeconds = doc["sleepTimeoutSeconds"] | 180;
  s.animationsEnabled = doc["animationsEnabled"] | true;
  s.autoRotateEnabled = doc["autoRotateEnabled"] | true;
  s.rotationIntervalSeconds = doc["rotationIntervalSeconds"] | 10;
  s.syncIntervalMinutes = doc["syncIntervalMinutes"] | 30;
  s.timezone = doc["timezone"] | "Europe/Paris";
  s.demoMode = doc["demoMode"] | true;
  return true;
}
}  // namespace

ConfigManager::ConfigManager(IPersistentStore& store) : store_(store) {}

bool ConfigManager::validate(AppSettings& s) const {
  bool changed = false;
  auto clamp = [&](int& v, int lo, int hi) {
    if (v < lo) {
      v = lo;
      changed = true;
    }
    if (v > hi) {
      v = hi;
      changed = true;
    }
  };
  clamp(s.brightnessPercent, 5, 100);
  clamp(s.sleepTimeoutSeconds, 0, 3600);
  clamp(s.rotationIntervalSeconds, 5, 300);
  clamp(s.syncIntervalMinutes, 5, 360);
  return !changed;
}

bool ConfigManager::migrate(AppSettings& s, int fromVersion) const {
  // Aucune migration necessaire pour l'instant (schema v1 unique). Point
  // d'extension documente dans docs/CONFIGURATION.md.
  (void)fromVersion;
  s.schemaVersion = AppSettings::kCurrentSchemaVersion;
  return true;
}

bool ConfigManager::load() {
  std::string raw;
  if (!store_.load(kStoreKey, raw)) {
    Logger::info("ConfigManager: aucune configuration existante, valeurs par defaut");
    settings_ = AppSettings();
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, raw);
  if (err) {
    Logger::error("ConfigManager: configuration corrompue (%s), reinitialisation", err.c_str());
    settings_ = AppSettings();
    return false;
  }

  AppSettings parsed;
  if (!fromJson(doc, parsed)) {
    Logger::error("ConfigManager: configuration invalide (champs essentiels absents), reinitialisation");
    settings_ = AppSettings();
    return false;
  }

  if (parsed.schemaVersion != AppSettings::kCurrentSchemaVersion) {
    Logger::warn("ConfigManager: migration schema v%d -> v%d", parsed.schemaVersion,
                 AppSettings::kCurrentSchemaVersion);
    migrate(parsed, parsed.schemaVersion);
  }

  validate(parsed);
  settings_ = parsed;
  return true;
}

bool ConfigManager::save() {
  validate(settings_);
  JsonDocument doc;
  toJson(settings_, doc, /*includeSecrets=*/true);
  std::string out;
  serializeJson(doc, out);
  bool ok = store_.save(kStoreKey, out);
  if (!ok) {
    Logger::error("ConfigManager: echec de sauvegarde de la configuration");
  }
  return ok;
}

void ConfigManager::resetToDefaults(bool persistNow) {
  settings_ = AppSettings();
  if (persistNow) {
    save();
  }
}

std::string ConfigManager::toPublicJson() const {
  JsonDocument doc;
  toJson(settings_, doc, /*includeSecrets=*/false);
  std::string out;
  serializeJson(doc, out);
  return out;
}

bool ConfigManager::applyJsonPatch(const std::string& json, std::string& outError) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, json);
  if (err) {
    outError = std::string("JSON invalide: ") + err.c_str();
    return false;
  }

  // Patch partiel : seuls les champs presents sont appliques.
  if (doc["wifiSsid"].is<const char*>()) settings_.wifiSsid = doc["wifiSsid"].as<std::string>();
  if (doc["wifiPassword"].is<const char*>()) settings_.wifiPassword = doc["wifiPassword"].as<std::string>();
  if (doc["psnUsername"].is<const char*>()) settings_.psnUsername = doc["psnUsername"].as<std::string>();
  if (doc["pocketPsnApiKey"].is<const char*>()) settings_.pocketPsnApiKey = doc["pocketPsnApiKey"].as<std::string>();
  if (doc["language"].is<const char*>()) {
    std::string lang = doc["language"].as<std::string>();
    settings_.language = (lang == "en") ? AppLanguage::kEnglish : AppLanguage::kFrench;
  }
  if (doc["brightnessPercent"].is<int>()) settings_.brightnessPercent = doc["brightnessPercent"];
  if (doc["sleepTimeoutSeconds"].is<int>()) settings_.sleepTimeoutSeconds = doc["sleepTimeoutSeconds"];
  if (doc["animationsEnabled"].is<bool>()) settings_.animationsEnabled = doc["animationsEnabled"];
  if (doc["autoRotateEnabled"].is<bool>()) settings_.autoRotateEnabled = doc["autoRotateEnabled"];
  if (doc["rotationIntervalSeconds"].is<int>()) settings_.rotationIntervalSeconds = doc["rotationIntervalSeconds"];
  if (doc["syncIntervalMinutes"].is<int>()) settings_.syncIntervalMinutes = doc["syncIntervalMinutes"];
  if (doc["timezone"].is<const char*>()) settings_.timezone = doc["timezone"].as<std::string>();
  if (doc["demoMode"].is<bool>()) settings_.demoMode = doc["demoMode"];

  validate(settings_);
  return true;
}
