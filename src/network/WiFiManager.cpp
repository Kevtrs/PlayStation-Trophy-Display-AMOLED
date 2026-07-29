#include "network/WiFiManager.h"

#include <WiFi.h>

#include "utils/Logger.h"

namespace {
// Point d'acces de secours : pas de mot de passe expose ici (voir
// docs/CONFIGURATION.md) -- le portail captif (tache #27) l'ouvrira sans
// authentification, comme la plupart des appareils IoT domestiques.
constexpr const char* kApSsid = "TrophyDisplay-Setup";
}  // namespace

void WiFiManager::begin(const std::string& ssid, const std::string& password) {
  // Evite de relancer une connexion deja active avec les memes
  // identifiants -- AppController appelle begin() a chaque sauvegarde de
  // reglages, meme sans rapport avec le Wi-Fi (ex: changement de
  // luminosite).
  if (hasCredentials_ && ssid == ssid_ && password == password_ &&
      (status_.state == WifiState::kConnected || status_.state == WifiState::kConnecting)) {
    return;
  }

  ssid_ = ssid;
  password_ = password;
  hasCredentials_ = !ssid.empty();
  consecutiveFailures_ = 0;
  pendingActionRequested_ = true;
}

void WiFiManager::requestReconnect() {
  consecutiveFailures_ = 0;
  pendingActionRequested_ = true;
}

void WiFiManager::forgetNetwork() {
  ssid_.clear();
  password_.clear();
  hasCredentials_ = false;
  consecutiveFailures_ = 0;
  WiFi.disconnect(true);
  pendingActionRequested_ = true;
}

void WiFiManager::startStationConnect(uint32_t nowMillis) {
  // AP_STA (pas STA seul) : garde le point d'acces de secours actif pendant
  // la tentative de connexion -- meme motif que startAccessPoint()
  // ci-dessous. Bug reel trouve le 2026-07-27 : WIFI_STA (station seule)
  // coupait immediatement le point d'acces des l'appui sur "Connecter"
  // dans le portail captif, avant meme de savoir si la connexion allait
  // reussir -- l'utilisateur perdait la page en plein milieu de la saisie
  // (ex: pseudo PSN pas encore renseigne). Le point d'acces est desactive
  // uniquement une fois la connexion reellement confirmee (voir poll(),
  // cas WifiState::kConnecting -> kConnected).
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(kApSsid);
  WiFi.begin(ssid_.c_str(), password_.c_str());
  status_.state = WifiState::kConnecting;
  status_.ssid = ssid_;
  status_.ipAddress.clear();
  connectStartedMillis_ = nowMillis;
  lastAttemptMillis_ = nowMillis;
  Logger::info("WiFiManager: connexion a '%s' en cours", ssid_.c_str());
}

void WiFiManager::startAccessPoint() {
  WiFi.disconnect(true);
  // AP_STA (pas AP seul) : necessaire pour pouvoir scanner/tenter une
  // connexion station pendant que le point d'acces de secours reste actif
  // (portail captif -- l'utilisateur doit pouvoir scanner les reseaux
  // depuis la page servie en mode AP).
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(kApSsid);
  status_.state = WifiState::kAccessPoint;
  status_.ssid.clear();
  status_.ipAddress = WiFi.softAPIP().toString().c_str();
  status_.rssiDbm = 0;
  Logger::warn("WiFiManager: bascule en point d'acces '%s' (%s)", kApSsid, status_.ipAddress.c_str());
}

uint32_t WiFiManager::backoffDelayMs() const {
  if (consecutiveFailures_ <= 0) return 0;
  int exponent = consecutiveFailures_ - 1;
  uint64_t delay = static_cast<uint64_t>(kReconnectBackoffBaseMs) << exponent;
  if (delay > kReconnectBackoffMaxMs) delay = kReconnectBackoffMaxMs;
  return static_cast<uint32_t>(delay);
}

void WiFiManager::requestScan() {
  if (scanState_ == ScanState::kScanning) return;
  // async=true : ne bloque jamais, resultat recupere via pollScan().
  WiFi.scanNetworks(/*async=*/true);
  scanState_ = ScanState::kScanning;
  scanResults_.clear();
}

void WiFiManager::pollScan() {
  if (scanState_ != ScanState::kScanning) return;
  int16_t result = WiFi.scanComplete();
  if (result == WIFI_SCAN_RUNNING) return;
  if (result < 0) {
    // WIFI_SCAN_FAILED ou autre code negatif : on considere le scan
    // termine (liste vide) plutot que de rester bloque indefiniment en
    // kScanning.
    scanState_ = ScanState::kDone;
    scanResults_.clear();
    Logger::warn("WiFiManager: echec du scan Wi-Fi (code %d)", result);
    return;
  }

  scanResults_.clear();
  scanResults_.reserve(result);
  for (int16_t i = 0; i < result; ++i) {
    NetworkScanResult entry;
    entry.ssid = WiFi.SSID(i).c_str();
    entry.rssiDbm = WiFi.RSSI(i);
    entry.requiresPassword = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    scanResults_.push_back(entry);
  }
  scanState_ = ScanState::kDone;
  WiFi.scanDelete();
  Logger::info("WiFiManager: scan termine, %d reseau(x) detecte(s)", result);
}

void WiFiManager::poll(uint32_t nowMillis) {
  pollScan();

  if (pendingActionRequested_) {
    pendingActionRequested_ = false;
    if (hasCredentials_) {
      startStationConnect(nowMillis);
    } else {
      startAccessPoint();
    }
    return;
  }

  switch (status_.state) {
    case WifiState::kConnecting: {
      if (WiFi.status() == WL_CONNECTED) {
        status_.state = WifiState::kConnected;
        status_.ipAddress = WiFi.localIP().toString().c_str();
        status_.rssiDbm = WiFi.RSSI();
        status_.reconnectAttempts = 0;
        consecutiveFailures_ = 0;
        // Coupe le point d'acces de secours maintenant que la connexion
        // reelle est confirmee -- il n'est garde actif que PENDANT la
        // tentative (voir startStationConnect()), pas indefiniment apres
        // une vraie connexion reussie.
        WiFi.mode(WIFI_STA);
        Logger::info("WiFiManager: connecte (%s, RSSI %d dBm)", status_.ipAddress.c_str(), status_.rssiDbm);
      } else if (nowMillis - connectStartedMillis_ >= kConnectTimeoutMs) {
        consecutiveFailures_++;
        Logger::warn("WiFiManager: timeout de connexion (%d echec(s) consecutif(s))", consecutiveFailures_);
        if (consecutiveFailures_ >= kMaxFailuresBeforeAp) {
          startAccessPoint();
        } else {
          status_.state = WifiState::kDisconnected;
          lastAttemptMillis_ = nowMillis;
        }
      }
      break;
    }
    case WifiState::kConnected: {
      if (WiFi.status() != WL_CONNECTED) {
        status_.state = WifiState::kDisconnected;
        status_.reconnectAttempts++;
        lastAttemptMillis_ = nowMillis;
        Logger::warn("WiFiManager: connexion perdue");
      } else {
        status_.rssiDbm = WiFi.RSSI();
      }
      break;
    }
    case WifiState::kDisconnected: {
      if (hasCredentials_ && (nowMillis - lastAttemptMillis_) >= backoffDelayMs()) {
        startStationConnect(nowMillis);
      }
      break;
    }
    case WifiState::kAccessPoint:
    case WifiState::kError:
    default:
      break;
  }
}
