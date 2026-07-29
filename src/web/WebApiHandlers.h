#pragma once

#include <cstdint>
#include <string>

#include "app/AppController.h"
#include "network/IWiFiManager.h"
#include "web/DiagnosticsSnapshot.h"

// Construction/analyse JSON des routes API du portail captif. Le contrat
// exact (noms de champs, unites) est celui du module Web UI
// (data/app.js/index.html) -- voir docs/WEB_UI_GAP_ANALYSIS.md pour le
// detail des ecarts avec l'implementation precedente et les decisions
// prises. Portable (ArduinoJson uniquement, aucune dependance
// WebServer/DNSServer) pour rester testable sans materiel : le transport
// HTTP reel vit dans CaptivePortalServer (firmware uniquement, voir
// src/web/CaptivePortalServer.h). Aucune logique metier ici (validation,
// persistance, reconfiguration) -- uniquement de la mise en forme/traduction
// JSON, la logique reste dans ConfigManager/AppController/WiFiManager/
// SyncService.
namespace WebApiHandlers {

// GET /api/status -- forme exacte attendue par data/app.js (renderStatus) :
// {"configured":bool,"offline":bool,"error":string|null,
//  "network":{"connected":bool,"ssid":string,"ip":string,"message":string},
//  "sync":{"state":string,"lastSync":string|"syncing"|null,"source":string}}
std::string buildStatusJson(const AppController& appController);

// GET /api/config -- forme exacte attendue par data/app.js (renderConfig).
// "pocketPsnKeyConfigured" (bool) est le seul champ lie a la cle API : la
// valeur elle-meme n'est jamais incluse ni ici ni dans buildConfigResponseJson().
std::string buildConfigJson(const AppController& appController);

// POST /api/config -- reponse enveloppee {"config":{...}} (renderConfig lit
// result.config), avec un "message" optionnel pour signaler une
// incompatibilite non bloquante (ex: langue non supportee, voir
// translateConfigPatch()) ou un redemarrage necessaire (voir
// configPatchRequiresRestart()). "pocketPsnApiKey" n'est jamais
// re-serialise dans la reponse, meme si le corps de la requete en
// contenait un (champ ecriture seule).
std::string buildConfigResponseJson(const AppController& appController, const std::string& warning);

// Traduit le contrat du module Web UI (ssid/brightness/sleepEnabled+sleepDelay/
// autoRotation/rotationDelay/animations/language/syncInterval) vers le
// JSON interne attendu par ConfigManager::applyJsonPatch() (wifiSsid/
// brightnessPercent/sleepTimeoutSeconds/autoRotateEnabled/
// rotationIntervalSeconds/animationsEnabled/language/syncIntervalMinutes).
// Ne valide/persiste rien elle-meme (ConfigManager s'en charge via
// AppController::applyConfigPatch()). outWarning est rempli (sans faire
// echouer la requete) pour une incompatibilite documentee et non
// bloquante : notre AppLanguage ne supporte que fr/en, contrairement aux 4
// langues proposees par le module Web UI (fr/en/es/de) -- une langue
// non supportee est ignoree (valeur precedente conservee) plutot que de
// planter ou de corrompre la configuration.
bool translateConfigPatch(const std::string& webConfigBody, std::string& outInternalJson, std::string& outWarning,
                          std::string& outError);

// Vrai si le patch interne deja traduit (sortie de translateConfigPatch(),
// format ConfigManager) touche a un champ qui n'est applique qu'au
// demarrage (psnUsername/pocketPsnApiKey -- voir ProviderFactory.h et
// AUDIT.md section 0ter) : l'appelant (CaptivePortalServer) doit alors
// programmer un redemarrage apres avoir persiste le changement, plutot que
// de pretendre qu'il est actif immediatement.
bool configPatchRequiresRestart(const std::string& internalJson);

// GET /api/wifi/scan -- {"networks":[{"ssid":...,"rssi":...,"secure":...}]}
// (data/app.js ne lit que "networks", le champ "status" ajoute ici est une
// extension ignoree sans risque par le front -- utile pour les tests, voir
// docs/WEB_UI_GAP_ANALYSIS.md).
std::string buildWifiScanJson(const IWiFiManager& wifi);

// Corps attendu pour POST /api/wifi/connect : {"ssid":"...","password":"..."}.
// Renvoie false et remplit outError si le JSON est invalide ou si "ssid"
// est absent/vide (le mot de passe peut etre vide -- reseau ouvert).
bool parseWifiConnectRequest(const std::string& body, std::string& outSsid, std::string& outPassword,
                             std::string& outError);

// Reponse generique {"message":"..."} (+ "ok" pour compatibilite interne)
// utilisee par POST /api/wifi/connect et POST /api/wifi/forget.
std::string buildSimpleResultJson(bool ok, const std::string& message = "");

// Reponse structuree pour une route dont la fonctionnalite sous-jacente
// n'est pas encore terminee (Pocket PSN, OTA, diagnostics...) --
// {"status":"not_implemented","message":"..."}. Ne simule jamais un succes
// (a utiliser avec un code HTTP non-2xx, ex: 501).
std::string buildNotImplementedJson(const std::string& message);

// POST /api/sync -- decision pure (n'appelle rien elle-meme) : le serveur
// web n'appelle jamais le provider directement, seulement
// AppController::requestManualSync() si cette fonction renvoie true (voir
// CaptivePortalServer::handleSync()).
bool shouldAcceptSyncRequest(SyncState currentState);
std::string buildSyncResponseJson(bool accepted, const std::string& message);

// POST /api/reset -- corps attendu {"confirm":true}. Renvoie false (et
// outConfirmed=false) si le JSON est invalide ; renvoie true avec
// outConfirmed refletant la valeur reelle du champ "confirm" (absent ou
// false => outConfirmed=false, la reinitialisation doit alors etre
// refusee par l'appelant).
bool parseResetConfirmation(const std::string& body, bool& outConfirmed, std::string& outError);

// GET /api/diagnostics -- ne contient jamais de secret (mot de passe
// Wi-Fi, cle API) : voir DiagnosticsSnapshot. Champs materiel absents
// (std::nullopt) rendus en JSON comme `null`.
std::string buildDiagnosticsJson(const DiagnosticsSnapshot& snapshot);

// Construit la partie portable de l'instantane (etat AppController :
// reseau, sync, cache) ; les champs materiel (heap/PSRAM/Flash/LittleFS/
// uptime) restent a std::nullopt ici -- a completer par l'appelant
// (CaptivePortalServer sur firmware ; laisses vides dans le simulateur,
// "non mesurable dans le simulateur" comme demande).
DiagnosticsSnapshot buildDiagnosticsSnapshot(const AppController& appController, uint32_t nowEpoch);

}  // namespace WebApiHandlers
