#include "web/CaptivePortalServer.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "utils/Logger.h"
#include "web/WebApiHandlers.h"

namespace {
constexpr uint16_t kDnsPort = 53;

// Repli technique embarque dans le binaire (pas dans LittleFS) : utilise
// uniquement si `data/index.html` (module Web UI) n'a pas encore ete
// televerse via `pio run -t uploadfs` -- sans lui, un appareil tout juste
// flashe afficherait une page vide/404 dans le portail captif avant le
// tout premier televersement de LittleFS. Volontairement minimal/sans
// design (voir mandat : ne pas travailler le design du module web) ;
// remplace entierement par data/index.html des que LittleFS est peuple.
const char kFallbackHtml[] = R"HTML(<!DOCTYPE html>
<html lang="fr"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Trophy Display -- Configuration (repli)</title></head>
<body style="font-family:sans-serif;max-width:420px;margin:24px auto;padding:0 12px">
<p><strong>Interface web (data/) non televersee.</strong> Cette page de
repli minimale permet neanmoins de configurer le Wi-Fi.</p>
<p id="s">Chargement...</p>
<h3>Reseaux</h3>
<ul id="nets"></ul>
<button onclick="scan()" type="button">Scanner</button>
<h3>Connexion</h3>
<input id="ssid" placeholder="SSID"><br>
<input id="pw" type="password" placeholder="Mot de passe"><br>
<button onclick="connect()" type="button">Se connecter</button>
<p id="r"></p>
<script>
async function api(p,o){const r=await fetch(p,o||{});return r.json().catch(()=>({}));}
async function status(){const d=await api('/api/status');document.getElementById('s').textContent='Wi-Fi: '+(d.network&&d.network.state||JSON.stringify(d.network));}
async function scan(){document.getElementById('nets').innerHTML='<li>Recherche...</li>';const d=await api('/api/wifi/scan');const ul=document.getElementById('nets');ul.innerHTML='';(d.networks||[]).forEach(function(n){const li=document.createElement('li');li.textContent=n.ssid+' ('+n.rssi+' dBm)';li.onclick=function(){document.getElementById('ssid').value=n.ssid;};ul.appendChild(li);});if(d.status==='scanning')setTimeout(scan,1000);}
async function connect(){const d=await api('/api/wifi/connect',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:document.getElementById('ssid').value,password:document.getElementById('pw').value})});document.getElementById('r').textContent=d.message||'OK';status();}
status();scan();
</script>
</body></html>
)HTML";
}  // namespace

CaptivePortalServer::CaptivePortalServer(AppController& appController) : appController_(appController) {}

void CaptivePortalServer::registerRoutes() {
  server_.on("/", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/index.html", HTTP_GET, [this]() { handleRoot(); });
  server_.serveStatic("/app.js", LittleFS, "/app.js");
  server_.serveStatic("/styles.css", LittleFS, "/styles.css");

  server_.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
  server_.on("/api/config", HTTP_GET, [this]() { handleConfigGet(); });
  server_.on("/api/config", HTTP_POST, [this]() { handleConfigPost(); });
  server_.on("/api/wifi/scan", HTTP_GET, [this]() { handleWifiScan(); });
  server_.on("/api/wifi/connect", HTTP_POST, [this]() { handleWifiConnect(); });
  server_.on("/api/wifi/forget", HTTP_POST, [this]() { handleWifiForget(); });
  server_.on("/api/profile/test", HTTP_POST, [this]() { handleProfileTest(); });
  server_.on("/api/sync", HTTP_POST, [this]() { handleSync(); });
  server_.on("/api/reboot", HTTP_POST, [this]() { handleReboot(); });
  server_.on("/api/reset", HTTP_POST, [this]() { handleReset(); });
  server_.on("/api/diagnostics", HTTP_GET, [this]() { handleDiagnostics(); });
  server_.onNotFound([this]() { handleNotFound(); });
}

void CaptivePortalServer::begin() {
  registerRoutes();
  server_.begin();
  Logger::info("CaptivePortalServer: serveur HTTP demarre (port 80)");
}

void CaptivePortalServer::poll() {
  // Le DNS captif ne doit rediriger que pendant que l'appareil est son
  // propre point d'acces -- en mode station (connecte au reseau de
  // l'utilisateur), hijacker les requetes DNS serait intrusif sur un
  // reseau qui n'appartient pas a l'appareil.
  bool isAp = appController_.wifi().status().state == WifiState::kAccessPoint;
  if (isAp && !dnsActive_) {
    dnsServer_.start(kDnsPort, "*", WiFi.softAPIP());
    dnsActive_ = true;
    Logger::info("CaptivePortalServer: portail captif DNS actif (mode point d'acces)");
  } else if (!isAp && dnsActive_) {
    dnsServer_.stop();
    dnsActive_ = false;
    Logger::info("CaptivePortalServer: portail captif DNS arrete (Wi-Fi station actif)");
  }

  if (dnsActive_) {
    dnsServer_.processNextRequest();
  }
  server_.handleClient();

  if (restartTimer_.consumeDue(millis())) {
    Logger::warn("CaptivePortalServer: redemarrage programme declenche");
    ESP.restart();
  }
}

void CaptivePortalServer::handleRoot() {
  if (LittleFS.exists("/index.html")) {
    File f = LittleFS.open("/index.html", "r");
    if (f) {
      server_.streamFile(f, "text/html");
      f.close();
      return;
    }
  }
  server_.send(200, "text/html", kFallbackHtml);
}

void CaptivePortalServer::handleStatus() {
  server_.send(200, "application/json", WebApiHandlers::buildStatusJson(appController_).c_str());
}

void CaptivePortalServer::handleConfigGet() {
  server_.send(200, "application/json", WebApiHandlers::buildConfigJson(appController_).c_str());
}

void CaptivePortalServer::handleConfigPost() {
  std::string body = server_.arg("plain").c_str();
  std::string internalJson, warning, error;
  if (!WebApiHandlers::translateConfigPatch(body, internalJson, warning, error)) {
    server_.send(400, "application/json", WebApiHandlers::buildSimpleResultJson(false, error).c_str());
    return;
  }

  std::string applyError;
  if (!appController_.applyConfigPatch(internalJson, applyError)) {
    server_.send(400, "application/json", WebApiHandlers::buildSimpleResultJson(false, applyError).c_str());
    return;
  }

  // psnUsername/pocketPsnApiKey ne sont appliques qu'au demarrage (voir
  // ProviderFactory.h et AUDIT.md section 0ter) : un redemarrage est donc
  // necessaire pour que le changement prenne effet. Repond avant de
  // programmer le redemarrage (voir handleReboot()/handleReset() pour le
  // meme motif), pour laisser le temps a la reponse HTTP de partir.
  if (WebApiHandlers::configPatchRequiresRestart(internalJson)) {
    server_.send(200, "application/json",
                 WebApiHandlers::buildConfigResponseJson(
                     appController_, "Configuration enregistree. Le Trophy Display va redemarrer pour appliquer "
                                     "les changements...")
                     .c_str());
    restartTimer_.schedule(millis(), kRestartDelayMs);
    return;
  }

  server_.send(200, "application/json", WebApiHandlers::buildConfigResponseJson(appController_, warning).c_str());
}

void CaptivePortalServer::handleWifiScan() {
  if (appController_.wifi().scanState() == ScanState::kIdle) {
    appController_.wifi().requestScan();
  }
  server_.send(200, "application/json", WebApiHandlers::buildWifiScanJson(appController_.wifi()).c_str());
}

void CaptivePortalServer::handleWifiConnect() {
  std::string body = server_.arg("plain").c_str();
  std::string ssid, password, error;
  if (!WebApiHandlers::parseWifiConnectRequest(body, ssid, password, error)) {
    server_.send(400, "application/json", WebApiHandlers::buildSimpleResultJson(false, error).c_str());
    return;
  }
  appController_.connectToWifi(ssid, password);
  server_.send(200, "application/json",
               WebApiHandlers::buildSimpleResultJson(true, "Connexion en cours").c_str());
}

void CaptivePortalServer::handleWifiForget() {
  appController_.forgetWifiNetwork();
  server_.send(200, "application/json", WebApiHandlers::buildSimpleResultJson(true, "Reseau oublie").c_str());
}

void CaptivePortalServer::sendNotImplemented(const std::string& message) {
  server_.send(501, "application/json", WebApiHandlers::buildNotImplementedJson(message).c_str());
}

void CaptivePortalServer::handleProfileTest() {
  // Laisse volontairement en not_implemented tant que l'outil Pocket PSN
  // Probe n'a pas fourni une vraie reponse et un schema confirme (voir
  // AUDIT.md et la consigne explicite de ne pas commencer cette route).
  sendNotImplemented(
      "Pocket PSN n'est pas encore integre (voir AUDIT.md) : impossible de tester un profil pour le moment.");
}

void CaptivePortalServer::handleSync() {
  // Ne touche jamais le provider directement : uniquement
  // AppController::requestManualSync() (qui delegue a SyncService). La
  // decision d'accepter/refuser est une fonction pure testable sans
  // materiel, voir WebApiHandlers::shouldAcceptSyncRequest().
  SyncState current = appController_.state().sync.state;
  if (!WebApiHandlers::shouldAcceptSyncRequest(current)) {
    server_.send(409, "application/json",
                 WebApiHandlers::buildSyncResponseJson(false, "Une synchronisation est deja en cours.").c_str());
    return;
  }
  appController_.requestManualSync();
  server_.send(202, "application/json",
               WebApiHandlers::buildSyncResponseJson(true, "Synchronisation lancee.").c_str());
}

void CaptivePortalServer::handleReboot() {
  // Repond avant de programmer le redemarrage (voir poll()), pour laisser
  // le temps a la reponse HTTP de partir sur le reseau.
  server_.send(200, "application/json",
               WebApiHandlers::buildSimpleResultJson(true, "Redemarrage dans quelques instants...").c_str());
  restartTimer_.schedule(millis(), kRestartDelayMs);
}

void CaptivePortalServer::handleReset() {
  std::string body = server_.arg("plain").c_str();
  bool confirmed = false;
  std::string error;
  if (!WebApiHandlers::parseResetConfirmation(body, confirmed, error) || !confirmed) {
    server_.send(400, "application/json",
                 WebApiHandlers::buildSimpleResultJson(false, "Confirmation requise (\"confirm\":true).").c_str());
    return;
  }

  appController_.factoryReset();
  server_.send(200, "application/json",
               WebApiHandlers::buildSimpleResultJson(true, "Configuration reinitialisee, redemarrage...").c_str());
  restartTimer_.schedule(millis(), kRestartDelayMs);
}

void CaptivePortalServer::handleDiagnostics() {
  DiagnosticsSnapshot snapshot = WebApiHandlers::buildDiagnosticsSnapshot(appController_, appController_.time().nowEpoch());

  snapshot.uptimeSeconds = millis() / 1000;
  snapshot.freeHeapBytes = ESP.getFreeHeap();
  snapshot.minFreeHeapBytes = ESP.getMinFreeHeap();
  if (psramFound()) {
    snapshot.psramTotalBytes = ESP.getPsramSize();
    snapshot.psramFreeBytes = ESP.getFreePsram();
  }
  snapshot.flashSizeBytes = ESP.getFlashChipSize();
  snapshot.appUsedBytes = ESP.getSketchSize();
  snapshot.littleFsTotalBytes = LittleFS.totalBytes();
  snapshot.littleFsUsedBytes = LittleFS.usedBytes();

  server_.send(200, "application/json", WebApiHandlers::buildDiagnosticsJson(snapshot).c_str());
}

void CaptivePortalServer::handleNotFound() {
  // Redirection captive standard : tout chemin inconnu en mode point
  // d'acces renvoie vers la page de configuration (voir mandat original,
  // "redirection vers 192.168.4.1 en mode AP"). En mode station, simple
  // 404 (pas de raison de rediriger sur le reseau de l'utilisateur).
  if (appController_.wifi().status().state == WifiState::kAccessPoint) {
    server_.sendHeader("Location", "http://192.168.4.1/", true);
    server_.send(302, "text/plain", "");
    return;
  }
  server_.send(404, "text/plain", "Not found");
}
