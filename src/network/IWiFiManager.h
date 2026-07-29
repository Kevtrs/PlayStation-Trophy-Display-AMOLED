#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "data/NetworkScanResult.h"
#include "data/NetworkStatus.h"

// Abstraction Wi-Fi portable (voir docs/IMPLEMENTATION_PLAN.md) : AppController
// ne connait jamais WiFi.h/l'ESP32 directement, uniquement cette interface.
// Deux implementations : WiFiManager (firmware, station/AP reels) et
// WiFiManagerStub (simulateur, aucun reseau reel).
enum class ScanState { kIdle, kScanning, kDone };

class IWiFiManager {
 public:
  virtual ~IWiFiManager() = default;

  // Demarre/relance la connexion avec ces identifiants. ssid vide =>
  // basculer directement en point d'acces de secours (aucune configuration
  // valide). Non bloquant : l'etat reel est observe via poll()/status().
  virtual void begin(const std::string& ssid, const std::string& password) = 0;

  // A appeler periodiquement depuis la boucle principale (jamais
  // bloquant) : fait avancer la machine a etats (connexion, detection de
  // deconnexion, reconnexion avec backoff, bascule AP apres echecs
  // repetes).
  virtual void poll(uint32_t nowMillis) = 0;

  // Oublie le reseau courant (efface les identifiants en memoire ici --
  // la suppression de la configuration persistante est a la charge de
  // l'appelant, voir AppController) et bascule en point d'acces.
  virtual void forgetNetwork() = 0;

  // Force une nouvelle tentative de connexion immediate (ignore le
  // backoff courant) avec les derniers identifiants connus.
  virtual void requestReconnect() = 0;

  virtual const NetworkStatus& status() const = 0;

  // Demande un scan des reseaux visibles (non bloquant : voir scanState()/
  // scanResults()). Sans effet si un scan est deja en cours.
  virtual void requestScan() = 0;

  virtual ScanState scanState() const = 0;

  // Valide uniquement quand scanState() == kDone. Vide sinon.
  virtual const std::vector<NetworkScanResult>& scanResults() const = 0;
};
