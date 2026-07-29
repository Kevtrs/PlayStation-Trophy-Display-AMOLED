#pragma once

#include "network/IWiFiManager.h"

// Implementation firmware reelle (ESP32 Arduino, WiFi.h). Machine a etats
// non bloquante : station en priorite, bascule automatique en point
// d'acces de secours si aucune configuration valide ou apres trop
// d'echecs consecutifs, reconnexion avec backoff exponentiel. Toutes les
// transitions d'etat sont pilotees depuis poll(nowMillis) : begin()/
// forgetNetwork()/requestReconnect() ne font que positionner une demande,
// traitee au prochain poll() avec le vrai horodatage (evite de calculer un
// delai ecoule depuis un horodatage factice).
class WiFiManager : public IWiFiManager {
 public:
  void begin(const std::string& ssid, const std::string& password) override;
  void poll(uint32_t nowMillis) override;
  void forgetNetwork() override;
  void requestReconnect() override;
  const NetworkStatus& status() const override { return status_; }

  void requestScan() override;
  ScanState scanState() const override { return scanState_; }
  const std::vector<NetworkScanResult>& scanResults() const override { return scanResults_; }

  static constexpr uint32_t kConnectTimeoutMs = 15000;
  static constexpr int kMaxFailuresBeforeAp = 5;
  static constexpr uint32_t kReconnectBackoffBaseMs = 2000;
  static constexpr uint32_t kReconnectBackoffMaxMs = 60000;

 private:
  void startStationConnect(uint32_t nowMillis);
  void startAccessPoint();
  uint32_t backoffDelayMs() const;
  void pollScan();

  NetworkStatus status_;
  std::string ssid_;
  std::string password_;
  bool hasCredentials_ = false;
  bool pendingActionRequested_ = false;

  uint32_t connectStartedMillis_ = 0;
  uint32_t lastAttemptMillis_ = 0;
  int consecutiveFailures_ = 0;

  ScanState scanState_ = ScanState::kIdle;
  std::vector<NetworkScanResult> scanResults_;
};
