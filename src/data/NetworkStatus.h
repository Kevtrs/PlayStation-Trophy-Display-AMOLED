#pragma once

#include <string>

// Etat reseau centralise, publie par WiFiManager (src/network/WiFiManager.h
// sur firmware ; simule via AppController::setSimulatedWifiState() sur PC).
enum class WifiState {
  kDisconnected,
  kConnecting,
  kConnected,
  kAccessPoint,
  kError,
};

struct NetworkStatus {
  WifiState state = WifiState::kDisconnected;
  std::string ssid;
  std::string ipAddress;
  int rssiDbm = 0;  // 0 si non connecte
  int reconnectAttempts = 0;
};
