#pragma once

#include <DNSServer.h>
#include <WebServer.h>

#include "app/AppController.h"
#include "services/PendingRestart.h"

// Portail captif + serveur HTTP (firmware uniquement -- WebServer.h/
// DNSServer.h ne sont pas portables, voir src/web/WebApiHandlers.h pour la
// logique JSON testable sans materiel). Ne bloque jamais la boucle
// principale : poll() ne fait que relayer vers
// DNSServer::processNextRequest()/WebServer::handleClient(), qui traitent
// au plus une requete par appel.
class CaptivePortalServer {
 public:
  explicit CaptivePortalServer(AppController& appController);

  // A appeler une fois, apres AppController::begin() (le Wi-Fi doit avoir
  // demarre, meme si l'etat final -- AP ou station -- n'est pas encore
  // connu).
  void begin();

  // A appeler a chaque iteration de la boucle principale.
  void poll();

 private:
  void registerRoutes();
  void handleRoot();
  void handleStatus();
  void handleConfigGet();
  void handleConfigPost();
  void handleWifiScan();
  void handleWifiConnect();
  void handleWifiForget();
  void handleProfileTest();
  void handleSync();
  void handleReboot();
  void handleReset();
  void handleDiagnostics();
  void handleNotFound();
  void sendNotImplemented(const std::string& message);

  AppController& appController_;
  WebServer server_{80};
  DNSServer dnsServer_;
  bool dnsActive_ = false;

  // Delai avant redemarrage (voir handleReboot()/handleReset()) : laisse
  // le temps a la reponse HTTP de partir sur le reseau avant ESP.restart().
  static constexpr uint32_t kRestartDelayMs = 800;
  PendingRestart restartTimer_;
};
