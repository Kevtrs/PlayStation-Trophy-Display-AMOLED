#pragma once

#include <string>

// Un reseau Wi-Fi detecte par un scan (voir IWiFiManager::scanResults()).
// Portable (aucune dependance WiFi.h) pour rester utilisable par
// WebApiHandlers (JSON) des deux cotes (firmware/simulateur).
struct NetworkScanResult {
  std::string ssid;
  int rssiDbm = 0;
  bool requiresPassword = false;
};
