#include "WiFiManagerStub.h"

void WiFiManagerStub::begin(const std::string& ssid, const std::string& password) {
  // Evite de relancer une connexion deja active avec les memes
  // identifiants -- AppController appelle begin() a chaque sauvegarde de
  // reglages, meme sans rapport avec le Wi-Fi.
  if (hasCredentials_ && ssid == ssid_ && password == password_ &&
      (status_.state == WifiState::kConnected || status_.state == WifiState::kConnecting)) {
    return;
  }

  ssid_ = ssid;
  password_ = password;
  hasCredentials_ = !ssid.empty();

  if (!hasCredentials_) {
    connectPending_ = false;
    status_ = NetworkStatus{};
    status_.state = WifiState::kAccessPoint;
    status_.ipAddress = "192.168.4.1";
    return;
  }

  status_.state = WifiState::kConnecting;
  status_.ssid = ssid_;
  status_.ipAddress.clear();
  connectPending_ = true;
  connectRequestedAtMillis_ = 0;  // fixe au prochain poll() avec le vrai nowMillis
}

void WiFiManagerStub::poll(uint32_t nowMillis) {
  if (scanState_ == ScanState::kScanning) {
    if (scanRequestedAtMillis_ == 0) scanRequestedAtMillis_ = nowMillis;
    if (nowMillis - scanRequestedAtMillis_ >= kSimulatedScanDelayMs) {
      // Reseaux fictifs stables (pas de materiel reel) -- suffisant pour
      // exercer le parcours complet scan -> selection -> connexion dans le
      // simulateur/les tests, voir --selftest.
      scanResults_ = {
          {"Simulateur", -40, true},
          {"SimuNet_Voisin", -68, true},
          {"SimuNet_Ouvert", -75, false},
      };
      scanState_ = ScanState::kDone;
    }
  }

  if (!connectPending_) return;
  if (connectRequestedAtMillis_ == 0) connectRequestedAtMillis_ = nowMillis;
  if (nowMillis - connectRequestedAtMillis_ >= kSimulatedConnectDelayMs) {
    connectPending_ = false;
    status_.state = WifiState::kConnected;
    status_.ipAddress = "127.0.0.1";
    status_.rssiDbm = -45;
    status_.reconnectAttempts = 0;
  }
}

void WiFiManagerStub::requestScan() {
  if (scanState_ == ScanState::kScanning) return;
  scanState_ = ScanState::kScanning;
  scanResults_.clear();
  scanRequestedAtMillis_ = 0;
}

void WiFiManagerStub::forgetNetwork() {
  ssid_.clear();
  password_.clear();
  hasCredentials_ = false;
  connectPending_ = false;
  status_ = NetworkStatus{};
  status_.state = WifiState::kAccessPoint;
  status_.ipAddress = "192.168.4.1";
}

void WiFiManagerStub::requestReconnect() {
  // Ne passe pas par begin() : celui-ci ignore une demande avec les memes
  // identifiants si deja connecte/en cours (voir begin()), ce qui
  // annulerait l'effet "force une reconnexion immediate" attendu ici.
  connectPending_ = false;
  hasCredentials_ = !ssid_.empty();
  if (!hasCredentials_) {
    status_.state = WifiState::kAccessPoint;
    status_.ipAddress = "192.168.4.1";
    return;
  }
  status_.state = WifiState::kConnecting;
  status_.ssid = ssid_;
  status_.ipAddress.clear();
  connectPending_ = true;
  connectRequestedAtMillis_ = 0;
}

void WiFiManagerStub::simulateConnected(const std::string& ssid) {
  connectPending_ = false;
  status_.state = WifiState::kConnected;
  status_.ssid = ssid;
  status_.ipAddress = "127.0.0.1";
  status_.rssiDbm = -40;
  status_.reconnectAttempts = 0;
}

void WiFiManagerStub::simulateDisconnected() {
  connectPending_ = false;
  status_.state = WifiState::kDisconnected;
  status_.ipAddress.clear();
  status_.rssiDbm = 0;
  status_.reconnectAttempts++;
}

void WiFiManagerStub::simulateAccessPoint() {
  connectPending_ = false;
  status_.state = WifiState::kAccessPoint;
  status_.ssid.clear();
  status_.ipAddress = "192.168.4.1";
  status_.rssiDbm = 0;
}

void WiFiManagerStub::simulateError() {
  connectPending_ = false;
  status_.state = WifiState::kError;
  status_.ipAddress.clear();
  status_.rssiDbm = 0;
}
