#include "network/PocketPsnHttpClient.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include <vector>

#include "utils/Logger.h"

namespace {
// Meme limite que PocketPsnProvider::kMaxResponseBytes (garde-fou, voir
// brief section 15) -- dupliquee ici volontairement : sans ce controle au
// niveau transport, https.getString() alloue integralement le corps annonce
// par Content-Length AVANT que PocketPsnProvider ne puisse jamais verifier
// sa taille, ce qui rend ce garde-fou inutile face a une reponse
// anormalement volumineuse (bug reel, cas limite decouvert lors de l'audit
// pre-materiel du 2026-07-21).
constexpr size_t kMaxResponseBytes = 64 * 1024;
}  // namespace

IPocketPsnHttpClient::Response PocketPsnHttpClient::post(const std::string& url, const std::string& body,
                                                          const std::string& contentType, int timeoutMs) {
  Response response;

  // TODO(non verifie) : remplacer setInsecure() par la verification du
  // certificat racine reel d'api.pocketpsn.com des qu'il aura ete observe
  // sur une connexion reelle. Utilise ici uniquement pour le POC, jamais
  // valide comme pratique finale (voir AUDIT.md).
  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(timeoutMs / 1000);

  HTTPClient https;
  https.setConnectTimeout(timeoutMs);
  https.setTimeout(timeoutMs);
  if (!https.begin(client, url.c_str())) {
    response.transportOk = false;
    return response;
  }
  https.addHeader("Content-Type", contentType.c_str());

  std::vector<uint8_t> bodyBytes(body.begin(), body.end());
  int code = https.POST(bodyBytes.data(), bodyBytes.size());

  if (code <= 0) {
    // Codes negatifs HTTPClient = erreur de transport (timeout, DNS, TLS...).
    response.transportOk = false;
    https.end();
    return response;
  }

  response.transportOk = true;
  response.httpStatus = code;
  response.contentType = https.header("Content-Type").c_str();

  // Verifie la taille annoncee AVANT de lire le corps : getString() alloue
  // la totalite du corps d'un coup, il faut donc refuser une lecture
  // anormalement grande ici, pas seulement apres coup (voir kMaxResponseBytes
  // ci-dessus). Content-Length inconnu/chunked (-1) : on laisse passer, la
  // vraie API Pocket PSN ne renvoie jamais de reponse chunked (691 octets
  // observes, voir docs/POCKETPSN_PROTOCOL.md).
  int declaredSize = https.getSize();
  if (declaredSize > 0 && static_cast<size_t>(declaredSize) > kMaxResponseBytes) {
    Logger::warn("PocketPsnHttpClient: reponse annoncee anormalement grande (%d octets), lecture refusee",
                 declaredSize);
    https.end();
    return response;  // body reste vide : traite comme une reponse vide par PocketPsnProvider
  }

  response.body = https.getString().c_str();
  https.end();
  return response;
}
