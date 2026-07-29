#pragma once

#include <cstdint>
#include <optional>
#include <string>

// Instantane de diagnostics (voir GET /api/diagnostics). Portable :
// les champs materiel (heap/PSRAM/Flash/LittleFS/uptime) ne sont jamais
// mesurables depuis WebApiHandlers (portable, sans acces materiel) --
// laisses a std::nullopt ici, remplis par CaptivePortalServer (firmware,
// ESP.getFreeHeap() etc.) avant serialisation. Le simulateur les laisse
// egalement a std::nullopt ("non mesurable dans le simulateur", voir
// consigne). Ne contient jamais de secret (mot de passe Wi-Fi, cle API).
struct DiagnosticsSnapshot {
  std::string firmwareVersion;

  std::optional<uint32_t> uptimeSeconds;
  std::optional<uint32_t> freeHeapBytes;
  std::optional<uint32_t> minFreeHeapBytes;
  std::optional<uint32_t> psramTotalBytes;
  std::optional<uint32_t> psramFreeBytes;
  std::optional<uint32_t> flashSizeBytes;
  std::optional<uint32_t> appUsedBytes;
  std::optional<uint32_t> littleFsTotalBytes;
  std::optional<uint32_t> littleFsUsedBytes;

  std::string wifiState;
  std::string ssid;
  std::string ipAddress;
  int rssi = 0;

  bool cacheAvailable = false;
  std::optional<int32_t> cacheAgeSeconds;

  std::string lastSyncState;
  uint32_t lastSyncTimestamp = 0;
  // Code HTTP brut de la derniere reponse Pocket PSN -- distinct de
  // lastErrorCode (code interne/erreur applicative) ; non mesurable tant
  // que PocketPsnProvider n'expose pas cette distinction separement (voir
  // docs/WEB_UI_GAP_ANALYSIS.md), toujours std::nullopt pour l'instant.
  std::optional<int> lastHttpStatus;
  int lastErrorCode = 0;
  int syncSuccessCount = 0;
  int syncFailureCount = 0;
};
