#pragma once

#include "network/IWiFiManager.h"

// Simulateur : aucun reseau reel. begin() simule une connexion aboutissant
// apres un court delai non bloquant (voir poll()) ; simulateXxx() est
// pilotable depuis simulator/DebugPanel.cpp pour exercer les scenarios
// (perte reseau, echec, bascule point d'acces) sans dependre de materiel.
class WiFiManagerStub : public IWiFiManager {
 public:
  void begin(const std::string& ssid, const std::string& password) override;
  void poll(uint32_t nowMillis) override;
  void forgetNetwork() override;
  void requestReconnect() override;
  const NetworkStatus& status() const override { return status_; }

  void requestScan() override;
  ScanState scanState() const override { return scanState_; }
  const std::vector<NetworkScanResult>& scanResults() const override { return scanResults_; }

  // --- API de simulation (simulator/DebugPanel.cpp uniquement) ---
  void simulateConnected(const std::string& ssid = "Simulateur");
  void simulateDisconnected();
  void simulateAccessPoint();
  void simulateError();

  static constexpr uint32_t kSimulatedConnectDelayMs = 600;
  static constexpr uint32_t kSimulatedScanDelayMs = 800;

 private:
  NetworkStatus status_;
  std::string ssid_;
  std::string password_;
  bool hasCredentials_ = false;
  bool connectPending_ = false;
  uint32_t connectRequestedAtMillis_ = 0;

  ScanState scanState_ = ScanState::kIdle;
  std::vector<NetworkScanResult> scanResults_;
  uint32_t scanRequestedAtMillis_ = 0;
};
